#include "storage/CaptureDatabase.h"
#include "core/GameIdentity.h"
#include "storage/GameIconCache.h"
#include "storage/GameMetadataBackfill.h"
#include "storage/CaptureQueries.h"
#include "storage/GameRowRepair.h"
#include "config/Paths.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <QDebug>

CaptureDatabase::CaptureDatabase(QString filePath, QObject* parent)
    : QObject(parent)
    , m_filePath(std::move(filePath))
{
}

CaptureDatabase::~CaptureDatabase()
{
    if (m_db.isOpen())
        m_db.close();
}

bool CaptureDatabase::open()
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("gamehq"));
    m_db.setDatabaseName(m_filePath);
    if (!m_db.open()) {
        qCritical() << "DB: open failed:" << m_db.lastError().text();
        return false;
    }
    QSqlQuery(QStringLiteral("PRAGMA foreign_keys = ON"), m_db);
    return migrate();
}

int CaptureDatabase::schemaVersion() const
{
    QSqlQuery q(QStringLiteral("PRAGMA user_version"), m_db);
    return q.next() ? q.value(0).toInt() : 0;
}

bool CaptureDatabase::migrate()
{
    const int version = schemaVersion();
    if (version < 1 && !applyV1())
        return false;
    if (version < 2 && !applyV2())
        return false;
    if (version < 3 && !applyV3())
        return false;
    if (!ensureGameMetadataColumns())
        return false;

    // One-time brand-path compatibility for databases created by earlier brands.
    // The filesystem migration renames the managed roots; keep absolute paths
    // in existing gallery rows aligned with their new locations.
    const QStringList brandPathUpdates = {
        QStringLiteral("UPDATE captures SET thumbnail_path = replace(thumbnail_path, '/playhq-data/', '/gamehq-data/') WHERE thumbnail_path LIKE '%/playhq-data/%'"),
        QStringLiteral("UPDATE captures SET thumbnail_path = replace(thumbnail_path, '\\playhq-data\\', '\\gamehq-data\\') WHERE thumbnail_path LIKE '%\\playhq-data\\%'"),
        QStringLiteral("UPDATE games SET icon_path = replace(icon_path, '/playhq-data/', '/gamehq-data/') WHERE icon_path LIKE '%/playhq-data/%'"),
        QStringLiteral("UPDATE games SET icon_path = replace(icon_path, '\\playhq-data\\', '\\gamehq-data\\') WHERE icon_path LIKE '%\\playhq-data\\%'"),
        QStringLiteral("UPDATE captures SET thumbnail_path = replace(thumbnail_path, '/saveplay-data/', '/gamehq-data/') WHERE thumbnail_path LIKE '%/saveplay-data/%'"),
        QStringLiteral("UPDATE captures SET thumbnail_path = replace(thumbnail_path, '\\saveplay-data\\', '\\gamehq-data\\') WHERE thumbnail_path LIKE '%\\saveplay-data\\%'"),
        QStringLiteral("UPDATE games SET icon_path = replace(icon_path, '/saveplay-data/', '/gamehq-data/') WHERE icon_path LIKE '%/saveplay-data/%'"),
        QStringLiteral("UPDATE games SET icon_path = replace(icon_path, '\\saveplay-data\\', '\\gamehq-data\\') WHERE icon_path LIKE '%\\saveplay-data\\%'"),
        QStringLiteral("UPDATE captures SET source = 'GameHQ' WHERE source IN ('PlayHQ', 'SavePlay')")
    };
    for (const QString& statement : brandPathUpdates) {
        QSqlQuery q(m_db);
        if (!q.exec(statement)) {
            qWarning() << "DB: brand-path migration failed:" << q.lastError().text();
            return false;
        }
    }

    // Collapse rows that became the same physical capture after a portable
    // folder rename. Keep the best thumbnail and preserve favorite state.
    struct CapturePathRow { int id; QString path; QString thumbnail; bool favorite; };
    QHash<QString, QVector<CapturePathRow>> captureGroups;
    QSqlQuery captureSelect(QStringLiteral(
        "SELECT id, file_path, thumbnail_path, is_favorite FROM captures WHERE deleted_at IS NULL"), m_db);
    while (captureSelect.next()) {
        CapturePathRow row{captureSelect.value(0).toInt(), captureSelect.value(1).toString(),
                           captureSelect.value(2).toString(), captureSelect.value(3).toBool()};
        const QString repaired = Paths::toStoredPath(Paths::repairMovedPath(row.path));
        captureGroups[repaired.toCaseFolded()].append(row);
    }
    captureSelect.finish();
    for (auto it = captureGroups.cbegin(); it != captureGroups.cend(); ++it) {
        const QVector<CapturePathRow>& rows = it.value();
        int winnerIndex = 0;
        const QString canonicalPath = Paths::toStoredPath(Paths::repairMovedPath(rows.first().path));
        bool winnerIsCanonical = rows.first().path.compare(canonicalPath, Qt::CaseInsensitive) == 0;
        for (int i = 1; i < rows.size(); ++i) {
            const bool candidateIsCanonical = rows[i].path.compare(canonicalPath, Qt::CaseInsensitive) == 0;
            const bool candidateHasThumb = QFileInfo::exists(Paths::repairMovedPath(rows[i].thumbnail));
            const bool winnerHasThumb = QFileInfo::exists(Paths::repairMovedPath(rows[winnerIndex].thumbnail));
            if ((candidateIsCanonical && !winnerIsCanonical)
                || (candidateIsCanonical == winnerIsCanonical && candidateHasThumb && !winnerHasThumb)) {
                winnerIndex = i;
                winnerIsCanonical = candidateIsCanonical;
            }
        }
        bool favorite = false;
        QString thumbnail;
        for (int i = 0; i < rows.size(); ++i) {
            favorite = favorite || rows[i].favorite;
            if (thumbnail.isEmpty() && QFileInfo::exists(Paths::repairMovedPath(rows[i].thumbnail)))
                thumbnail = Paths::toStoredPath(Paths::repairMovedPath(rows[i].thumbnail));
            if (i == winnerIndex)
                continue;
            QSqlQuery tombstone(m_db);
            tombstone.prepare(QStringLiteral("UPDATE captures SET deleted_at = :now WHERE id = :id"));
            tombstone.bindValue(QStringLiteral(":now"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            tombstone.bindValue(QStringLiteral(":id"), rows[i].id);
            tombstone.exec();
        }
        QSqlQuery update(m_db);
        update.prepare(QStringLiteral(
            "UPDATE captures SET file_path = :path, thumbnail_path = :thumb, is_favorite = :favorite WHERE id = :id"));
        update.bindValue(QStringLiteral(":path"), canonicalPath);
        update.bindValue(QStringLiteral(":thumb"), thumbnail.isEmpty() ? QVariant() : QVariant(thumbnail));
        update.bindValue(QStringLiteral(":favorite"), favorite ? 1 : 0);
        update.bindValue(QStringLiteral(":id"), rows[winnerIndex].id);
        update.exec();
    }

    // Normalize remaining package-owned paths after a move/rename.
    const struct { const char* table; const char* id; const char* column; } pathColumns[] = {
        { "captures", "id", "thumbnail_path" },
        { "games", "id", "icon_path" }, { "folders", "id", "path" }
    };
    for (const auto& spec : pathColumns) {
        QSqlQuery select(m_db);
        const QString sql = QStringLiteral("SELECT %1, %2 FROM %3 WHERE %2 IS NOT NULL AND %2 != ''")
                                .arg(QString::fromLatin1(spec.id), QString::fromLatin1(spec.column),
                                     QString::fromLatin1(spec.table));
        if (!select.exec(sql))
            continue;
        while (select.next()) {
            const QString oldValue = select.value(1).toString();
            const QString newValue = Paths::toStoredPath(Paths::repairMovedPath(oldValue));
            if (newValue == oldValue)
                continue;
            QSqlQuery update(m_db);
            update.prepare(QStringLiteral("UPDATE %1 SET %2 = :path WHERE %3 = :id")
                               .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.column),
                                    QString::fromLatin1(spec.id)));
            update.bindValue(QStringLiteral(":path"), newValue);
            update.bindValue(QStringLiteral(":id"), select.value(0));
            update.exec();
        }
    }
    GameRowRepair::normalizeDuplicateNames(m_db);
    GameMetadataBackfill::run(m_db);
    qInfo() << "DB: schema version" << schemaVersion();
    return true;
}

QVector<CaptureRecord> CaptureDatabase::listCaptures(const QString& category, int gameId) const
{
    return CaptureQueries::listCaptures(m_db, category, gameId);
}

bool CaptureDatabase::hasCapture(const QString& filePath) const
{
    return CaptureQueries::hasCapture(m_db, filePath);
}

bool CaptureDatabase::hasCapturesForGame(int gameId) const
{
    return CaptureQueries::hasCapturesForGame(m_db, gameId);
}

int CaptureDatabase::insertCapture(const QString& filePath, const QString& type,
                                   const QString& gameName, const QString& createdAt,
                                   const QString& source, const QString& executablePath)
{
    if (hasCapture(filePath))
        return -1;
    const int gameId = findOrCreateGame(gameName, executablePath);
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO captures (file_path, type, game_id, created_at, source) "
        "VALUES (:path, :type, :game, :created, :source)"));
    q.bindValue(QStringLiteral(":path"), Paths::toStoredPath(filePath));
    q.bindValue(QStringLiteral(":type"), type);
    q.bindValue(QStringLiteral(":game"), gameId >= 0 ? QVariant(gameId) : QVariant());
    q.bindValue(QStringLiteral(":created"), createdAt);
    q.bindValue(QStringLiteral(":source"), source);
    if (!q.exec()) {
        qWarning() << "DB: insertCapture failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool CaptureDatabase::setFavorite(int captureId, bool favorite)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE captures SET is_favorite = :f WHERE id = :id"));
    q.bindValue(QStringLiteral(":f"), favorite ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), captureId);
    return q.exec();
}

bool CaptureDatabase::setThumbnail(int captureId, const QString& thumbnailPath)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE captures SET thumbnail_path = :t WHERE id = :id"));
    q.bindValue(QStringLiteral(":t"), Paths::toStoredPath(thumbnailPath));
    q.bindValue(QStringLiteral(":id"), captureId);
    return q.exec();
}

QString CaptureDatabase::thumbnailForCapture(const QString& filePath) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT thumbnail_path FROM captures WHERE file_path = :path AND deleted_at IS NULL LIMIT 1"));
    q.bindValue(QStringLiteral(":path"), Paths::toStoredPath(filePath));
    return q.exec() && q.next() ? Paths::repairMovedPath(q.value(0).toString()) : QString();
}

bool CaptureDatabase::setThumbnailForCapture(const QString& filePath, const QString& thumbnailPath)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE captures SET thumbnail_path = :thumb WHERE file_path = :path AND deleted_at IS NULL"));
    q.bindValue(QStringLiteral(":thumb"), Paths::toStoredPath(thumbnailPath));
    q.bindValue(QStringLiteral(":path"), Paths::toStoredPath(filePath));
    return q.exec();
}

bool CaptureDatabase::deleteCapture(int captureId)
{
    // Soft-delete: the row is tombstoned (deleted_at) so it drops out of every
    // listing; the physical file is removed by the caller (AppController).
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE captures SET deleted_at = :t WHERE id = :id"));
    q.bindValue(QStringLiteral(":t"),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":id"), captureId);
    if (!q.exec()) {
        qWarning() << "DB: deleteCapture failed" << q.lastError().text();
        return false;
    }
    return true;
}

QVector<GameEntry> CaptureDatabase::listGames() const
{
    return CaptureQueries::listGames(m_db);
}

bool CaptureDatabase::rememberGameExecutable(const QString& displayName,
                                             const QString& executablePath)
{
    if (displayName.isEmpty() || executablePath.isEmpty() || !QFileInfo::exists(executablePath))
        return false;

    const QString key = GameIdentity::key(displayName);
    int gameId = -1;
    QString oldExecutable;
    QString oldIcon;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, display_name, executable_path, icon_path FROM games"));
    if (q.exec()) {
        while (q.next()) {
            if (GameIdentity::key(q.value(1).toString()) != key)
                continue;
            gameId = q.value(0).toInt();
            oldExecutable = q.value(2).toString();
            oldIcon = q.value(3).toString();
            break;
        }
    }

    if (gameId < 0)
        gameId = findOrCreateGame(displayName, executablePath);
    else
        updateGameExecutable(gameId, executablePath);

    if (gameId < 0)
        return false;

    QSqlQuery after(m_db);
    after.prepare(QStringLiteral("SELECT executable_path, icon_path FROM games WHERE id = :id"));
    after.bindValue(QStringLiteral(":id"), gameId);
    if (!after.exec() || !after.next())
        return false;

    return oldExecutable != after.value(0).toString()
           || (oldIcon.isEmpty() && !after.value(1).toString().isEmpty());
}

bool CaptureDatabase::seedDefaultBindings()
{
    QSqlQuery count(QStringLiteral("SELECT COUNT(*) FROM bindings"), m_db);
    if (count.next() && count.value(0).toInt() > 0)
        return true;   // already seeded / user-edited — never overwrite

    struct Def { const char* device; const char* code; const char* action;
                 const char* press; int hold; };
    static const Def defaults[] = {
        { "controller", "Share", "screenshot",     "tap",  0 },
        { "controller", "Share", "save_replay",    "hold", 2000 },
        { "controller", "PS",    "overlay_toggle", "tap",  0 },
        { "controller", "Circle","overlay_hide",   "tap",  0 },
        { "keyboard",   "Ctrl+Shift+G", "overlay_toggle", "combo", 0 },
        { "keyboard",   "Ctrl+Shift+S", "screenshot",     "combo", 0 },
        { "keyboard",   "Ctrl+Shift+R", "save_replay",    "combo", 0 },
    };

    if (!m_db.transaction())
        return false;
    for (const Def& d : defaults) {
        QSqlQuery q(m_db);
        q.prepare(QStringLiteral(
            "INSERT INTO bindings (device_type, input_code, action, press_type, hold_ms) "
            "VALUES (:dev, :code, :act, :press, :hold)"));
        q.bindValue(QStringLiteral(":dev"), QString::fromLatin1(d.device));
        q.bindValue(QStringLiteral(":code"), QString::fromLatin1(d.code));
        q.bindValue(QStringLiteral(":act"), QString::fromLatin1(d.action));
        q.bindValue(QStringLiteral(":press"), QString::fromLatin1(d.press));
        q.bindValue(QStringLiteral(":hold"), d.hold > 0 ? QVariant(d.hold) : QVariant());
        if (!q.exec()) {
            qWarning() << "DB: seedDefaultBindings failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }
    const bool ok = m_db.commit();
    if (ok)
        qInfo() << "DB: seeded default input bindings";
    return ok;
}

QVector<BindingRow> CaptureDatabase::listBindings() const
{
    QVector<BindingRow> out;
    QSqlQuery q(QStringLiteral(
        "SELECT device_type, input_code, action, press_type, hold_ms FROM bindings "
        "ORDER BY id"), m_db);
    while (q.next()) {
        BindingRow r;
        r.deviceType = q.value(0).toString();
        r.inputCode  = q.value(1).toString();
        r.action     = q.value(2).toString();
        r.pressType  = q.value(3).toString();
        r.holdMs     = q.value(4).isNull() ? 0 : q.value(4).toInt();
        out.append(r);
    }
    return out;
}

QVector<BindingOverrideRow> CaptureDatabase::listBindingOverrides() const
{
    QVector<BindingOverrideRow> out;
    QSqlQuery q(QStringLiteral(
        "SELECT device_group, device_profile, action_id, slot, trigger_code, "
        "activation, hold_ms, unbound FROM binding_overrides ORDER BY id"), m_db);
    while (q.next()) {
        BindingOverrideRow r;
        r.deviceGroup   = q.value(0).toString();
        r.deviceProfile = q.value(1).toString();
        r.actionId      = q.value(2).toString();
        r.slot          = q.value(3).toInt();
        r.triggerCode   = q.value(4).toString();
        r.activation    = q.value(5).toString();
        r.holdMs        = q.value(6).isNull() ? 0 : q.value(6).toInt();
        r.unbound       = q.value(7).toInt() != 0;
        out.append(r);
    }
    return out;
}

bool CaptureDatabase::upsertBindingOverride(const BindingOverrideRow& row)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO binding_overrides "
        "(device_group, device_profile, action_id, slot, trigger_code, activation, hold_ms, unbound) "
        "VALUES (:group, :profile, :action, :slot, :trigger, :activation, :hold, :unbound) "
        "ON CONFLICT(device_group, device_profile, action_id, slot) DO UPDATE SET "
        "trigger_code = excluded.trigger_code, activation = excluded.activation, "
        "hold_ms = excluded.hold_ms, unbound = excluded.unbound"));
    q.bindValue(QStringLiteral(":group"), row.deviceGroup);
    q.bindValue(QStringLiteral(":profile"), row.deviceProfile);
    q.bindValue(QStringLiteral(":action"), row.actionId);
    q.bindValue(QStringLiteral(":slot"), row.slot);
    q.bindValue(QStringLiteral(":trigger"), row.triggerCode.isEmpty() ? QVariant() : row.triggerCode);
    q.bindValue(QStringLiteral(":activation"), row.activation);
    q.bindValue(QStringLiteral(":hold"), row.holdMs > 0 ? QVariant(row.holdMs) : QVariant());
    q.bindValue(QStringLiteral(":unbound"), row.unbound ? 1 : 0);
    if (!q.exec()) {
        qWarning() << "DB: upsertBindingOverride failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool CaptureDatabase::clearBindingOverride(const QString& deviceGroup, const QString& deviceProfile,
                                            const QString& actionId, int slot)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM binding_overrides WHERE device_group = :group AND device_profile = :profile "
        "AND action_id = :action AND slot = :slot"));
    q.bindValue(QStringLiteral(":group"), deviceGroup);
    q.bindValue(QStringLiteral(":profile"), deviceProfile);
    q.bindValue(QStringLiteral(":action"), actionId);
    q.bindValue(QStringLiteral(":slot"), slot);
    return q.exec();
}

bool CaptureDatabase::clearBindingOverridesForGroup(const QString& deviceGroup)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM binding_overrides WHERE device_group = :group"));
    q.bindValue(QStringLiteral(":group"), deviceGroup);
    return q.exec();
}

bool CaptureDatabase::clearBindingOverridesForProfile(const QString& deviceGroup,
                                                       const QString& deviceProfile)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM binding_overrides WHERE device_group = :group AND device_profile = :profile"));
    q.bindValue(QStringLiteral(":group"), deviceGroup);
    q.bindValue(QStringLiteral(":profile"), deviceProfile);
    return q.exec();
}

bool CaptureDatabase::clearAllBindingOverrides()
{
    QSqlQuery q(m_db);
    return q.exec(QStringLiteral("DELETE FROM binding_overrides"));
}

QStringList CaptureDatabase::watchedFolders() const
{
    QStringList out;
    QSqlQuery q(QStringLiteral("SELECT path FROM folders WHERE is_watched = 1"), m_db);
    while (q.next())
        out.append(Paths::repairMovedPath(q.value(0).toString()));
    return out;
}

bool CaptureDatabase::addWatchedFolder(const QString& path, const QString& source)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO folders (path, source, is_watched) VALUES (:p, :s, 1)"));
    q.bindValue(QStringLiteral(":p"), Paths::toStoredPath(path));
    q.bindValue(QStringLiteral(":s"), source);
    return q.exec();
}

bool CaptureDatabase::removeWatchedFolder(const QString& path)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM folders WHERE path = :p"));
    q.bindValue(QStringLiteral(":p"), Paths::toStoredPath(path));
    return q.exec();
}

bool CaptureDatabase::ensureGameMetadataColumns()
{
    QStringList columns;
    QSqlQuery info(QStringLiteral("PRAGMA table_info(games)"), m_db);
    while (info.next())
        columns.append(info.value(1).toString());

    struct Column { const char* name; const char* sql; };
    const Column wanted[] = {
        { "executable_path", "ALTER TABLE games ADD COLUMN executable_path TEXT" },
        { "icon_path",       "ALTER TABLE games ADD COLUMN icon_path TEXT" },
        { "last_seen_at",    "ALTER TABLE games ADD COLUMN last_seen_at TEXT" },
    };
    for (const Column& c : wanted) {
        if (columns.contains(QString::fromLatin1(c.name)))
            continue;
        QSqlQuery alter(m_db);
        if (!alter.exec(QString::fromLatin1(c.sql))) {
            qCritical() << "DB: could not add games." << c.name << alter.lastError().text();
            return false;
        }
        qInfo() << "DB: added games." << c.name;
    }
    return true;
}

int CaptureDatabase::findOrCreateGame(const QString& displayName, const QString& executablePath)
{
    const QString name = displayName.isEmpty() ? QStringLiteral("Unknown Game") : displayName;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM games WHERE display_name = :n"));
    q.bindValue(QStringLiteral(":n"), name);
    if (q.exec() && q.next()) {
        const int id = q.value(0).toInt();
        updateGameExecutable(id, executablePath);
        return id;
    }

    q.prepare(QStringLiteral("SELECT id, display_name FROM games"));
    if (q.exec()) {
        const QString key = GameIdentity::key(name);
        while (q.next()) {
            const int id = q.value(0).toInt();
            const QString existing = q.value(1).toString();
            if (GameIdentity::key(existing) != key)
                continue;

            if (GameRowRepair::isBetterDisplayName(name, existing)) {
                QSqlQuery updateName(m_db);
                updateName.prepare(QStringLiteral(
                    "UPDATE games SET display_name = :name WHERE id = :id"));
                updateName.bindValue(QStringLiteral(":name"), name);
                updateName.bindValue(QStringLiteral(":id"), id);
                if (!updateName.exec())
                    qWarning() << "DB: could not update game display name:"
                               << updateName.lastError().text();
            }
            updateGameExecutable(id, executablePath);
            return id;
        }
    }

    q.prepare(QStringLiteral(
        "INSERT INTO games (display_name, executable_path, icon_path, created_at) "
        "VALUES (:n, :exe, :icon, :c)"));
    const QString iconPath = GameIconCache::iconPathForExecutable(executablePath);
    q.bindValue(QStringLiteral(":n"), name);
    q.bindValue(QStringLiteral(":exe"), executablePath.isEmpty() ? QVariant() : QVariant(executablePath));
    q.bindValue(QStringLiteral(":icon"), iconPath.isEmpty() ? QVariant() : QVariant(iconPath));
    q.bindValue(QStringLiteral(":c"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "DB: findOrCreateGame failed:" << q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

void CaptureDatabase::updateGameExecutable(int gameId, const QString& executablePath)
{
    if (gameId < 0 || executablePath.isEmpty() || !QFileInfo::exists(executablePath))
        return;

    const QString iconPath = GameIconCache::iconPathForExecutable(executablePath);
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE games SET executable_path = :exe, icon_path = COALESCE(:icon, icon_path), "
        "last_seen_at = :seen WHERE id = :id"));
    q.bindValue(QStringLiteral(":exe"), executablePath);
    q.bindValue(QStringLiteral(":icon"), iconPath.isEmpty() ? QVariant() : QVariant(iconPath));
    q.bindValue(QStringLiteral(":seen"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":id"), gameId);
    if (!q.exec())
        qWarning() << "DB: could not update game executable/icon:" << q.lastError().text();
}

bool CaptureDatabase::applyV1()
{
    const QStringList statements = {
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS games (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            display_name    TEXT NOT NULL,
            process_name    TEXT UNIQUE,
            executable_path TEXT,
            icon_path       TEXT,
            created_at      TEXT NOT NULL,
            last_seen_at    TEXT,
            is_whitelisted  INTEGER NOT NULL DEFAULT 0))"),
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS captures (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path      TEXT NOT NULL UNIQUE,
            type           TEXT NOT NULL CHECK(type IN ('screenshot','video')),
            game_id        INTEGER REFERENCES games(id),
            process_name   TEXT,
            window_title   TEXT,
            created_at     TEXT NOT NULL,
            duration_ms    INTEGER,
            width          INTEGER,
            height         INTEGER,
            fps            INTEGER,
            codec          TEXT,
            bitrate        INTEGER,
            is_favorite    INTEGER NOT NULL DEFAULT 0,
            source         TEXT NOT NULL DEFAULT 'GameHQ',
            thumbnail_path TEXT,
            deleted_at     TEXT))"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_captures_game ON captures(game_id)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_captures_created ON captures(created_at)"),
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS settings (
            key   TEXT PRIMARY KEY,
            value TEXT))"),
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS bindings (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            device_type TEXT NOT NULL CHECK(device_type IN ('keyboard','controller')),
            input_code  TEXT NOT NULL,
            action      TEXT NOT NULL,
            press_type  TEXT NOT NULL DEFAULT 'tap' CHECK(press_type IN ('tap','hold','combo')),
            hold_ms     INTEGER))"),
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS folders (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            path       TEXT NOT NULL UNIQUE,
            source     TEXT NOT NULL DEFAULT 'Custom',
            is_watched INTEGER NOT NULL DEFAULT 1))"),
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS sound_settings (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            event_name TEXT NOT NULL UNIQUE,
            enabled    INTEGER NOT NULL DEFAULT 1,
            volume     INTEGER NOT NULL DEFAULT 80,
            sound_file TEXT))"),
    };

    if (!m_db.transaction()) {
        qCritical() << "DB: cannot start migration transaction";
        return false;
    }
    for (const QString& sql : statements) {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            qCritical() << "DB: migration v1 failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }
    QSqlQuery(QStringLiteral("PRAGMA user_version = 1"), m_db);
    return m_db.commit();
}

bool CaptureDatabase::applyV2()
{
    // Additive only — v1's tables (including the legacy `bindings` seed data)
    // are untouched. Built-in trigger defaults live in code, so this table
    // starts empty on every upgrade; there is nothing to migrate out of v1.
    const QStringList statements = {
        QStringLiteral(R"(CREATE TABLE IF NOT EXISTS binding_overrides (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            device_group   TEXT NOT NULL CHECK(device_group IN ('keyboard','controller')),
            device_profile TEXT NOT NULL DEFAULT '',
            action_id      TEXT NOT NULL,
            slot           INTEGER NOT NULL DEFAULT 1 CHECK(slot IN (1,2)),
            trigger_code   TEXT,
            activation     TEXT NOT NULL DEFAULT 'press' CHECK(activation IN ('press','tap','hold','double_tap')),
            hold_ms        INTEGER,
            unbound        INTEGER NOT NULL DEFAULT 0 CHECK(unbound IN (0,1))))"),
        QStringLiteral(
            "CREATE UNIQUE INDEX IF NOT EXISTS idx_binding_overrides_scope "
            "ON binding_overrides(device_group, device_profile, action_id, slot)"),
    };

    if (!m_db.transaction()) {
        qCritical() << "DB: cannot start migration transaction";
        return false;
    }
    for (const QString& sql : statements) {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            qCritical() << "DB: migration v2 failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }
    QSqlQuery(QStringLiteral("PRAGMA user_version = 2"), m_db);
    return m_db.commit();
}

bool CaptureDatabase::applyV3()
{
    // SQLite cannot widen a CHECK constraint in place. Rebuild the sparse
    // override table so extra mouse buttons can use the same canonical model.
    const QStringList statements = {
        QStringLiteral("DROP INDEX IF EXISTS idx_binding_overrides_scope"),
        QStringLiteral(R"(CREATE TABLE binding_overrides_v3 (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            device_group   TEXT NOT NULL CHECK(device_group IN ('keyboard','controller','mouse')),
            device_profile TEXT NOT NULL DEFAULT '',
            action_id      TEXT NOT NULL,
            slot           INTEGER NOT NULL DEFAULT 1 CHECK(slot IN (1,2)),
            trigger_code   TEXT,
            activation     TEXT NOT NULL DEFAULT 'press' CHECK(activation IN ('press','tap','hold','double_tap')),
            hold_ms        INTEGER,
            unbound        INTEGER NOT NULL DEFAULT 0 CHECK(unbound IN (0,1))))"),
        QStringLiteral(R"(INSERT INTO binding_overrides_v3
            (id, device_group, device_profile, action_id, slot, trigger_code, activation, hold_ms, unbound)
            SELECT id, device_group, device_profile, action_id, slot, trigger_code, activation, hold_ms, unbound
            FROM binding_overrides)"),
        QStringLiteral("DROP TABLE binding_overrides"),
        QStringLiteral("ALTER TABLE binding_overrides_v3 RENAME TO binding_overrides"),
        QStringLiteral("CREATE UNIQUE INDEX idx_binding_overrides_scope ON binding_overrides(device_group, device_profile, action_id, slot)"),
    };

    if (!m_db.transaction())
        return false;
    for (const QString& sql : statements) {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            qCritical() << "DB: migration v3 failed:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }
    QSqlQuery(QStringLiteral("PRAGMA user_version = 3"), m_db);
    return m_db.commit();
}
