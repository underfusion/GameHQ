#include "storage/GameMetadataBackfill.h"

#include "config/Paths.h"
#include "core/GameIdentity.h"
#include "storage/GameIconCache.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QVariant>
#include <QVector>
#include <QDebug>

namespace
{
void updateGameExecutable(QSqlDatabase& db, int gameId, const QString& executablePath)
{
    if (gameId < 0 || executablePath.isEmpty() || !QFileInfo::exists(executablePath))
        return;

    const QString iconPath = GameIconCache::iconPathForExecutable(executablePath);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE games SET executable_path = :exe, icon_path = COALESCE(:icon, icon_path), "
        "last_seen_at = :seen WHERE id = :id"));
    q.bindValue(QStringLiteral(":exe"), executablePath);
    q.bindValue(QStringLiteral(":icon"), iconPath.isEmpty() ? QVariant() : QVariant(iconPath));
    q.bindValue(QStringLiteral(":seen"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":id"), gameId);
    if (!q.exec())
        qWarning() << "DB: could not backfill game executable/icon:" << q.lastError().text();
}
} // namespace

void GameMetadataBackfill::run(QSqlDatabase& db)
{
    QFile log(Paths::logsDir() + QStringLiteral("/gamehq.log"));
    if (!log.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    struct MissingGame { int id; QString key; QString name; };
    QVector<MissingGame> missing;
    QSqlQuery games(db);
    games.prepare(QStringLiteral(
        "SELECT id, display_name FROM games "
        "WHERE (executable_path IS NULL OR executable_path = '') "
        "   OR (icon_path IS NULL OR icon_path = '')"));
    if (!games.exec())
        return;
    while (games.next()) {
        missing.append({ games.value(0).toInt(),
                         GameIdentity::key(games.value(1).toString()),
                         games.value(1).toString() });
    }
    if (missing.isEmpty())
        return;

    static const QRegularExpression re(
        QStringLiteral(R"(GameDetector title candidates for .* \| steam: ([^|]+) \| window: ([^|]+) \| ProductName: ([^|]+) \| FileDescription: ([^|]+) \| fromExe: ([^|]+) \| path: (.+)$)"));

    int updates = 0;
    QTextStream in(&log);
    while (!in.atEnd() && !missing.isEmpty()) {
        const QString line = in.readLine();
        const auto m = re.match(line);
        if (!m.hasMatch())
            continue;

        const QString path = m.captured(6).trimmed();
        if (path == QLatin1String("<none>") || !QFileInfo::exists(path))
            continue;

        const QString steamName = m.captured(1).trimmed();
        if (steamName.isEmpty() || steamName == QLatin1String("<none>"))
            continue;
        const QString candidateKey = GameIdentity::key(steamName);

        for (int i = 0; i < missing.size(); ++i) {
            if (candidateKey != missing.at(i).key)
                continue;

            updateGameExecutable(db, missing.at(i).id, path);
            qInfo() << "DB: backfilled game icon from historical detection for"
                    << missing.at(i).name;
            missing.removeAt(i);
            ++updates;
            break;
        }
    }

    if (updates > 0)
        qInfo() << "DB: backfilled" << updates << "game icon(s) from gamehq.log";
}
