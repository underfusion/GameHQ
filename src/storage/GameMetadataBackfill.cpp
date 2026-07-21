#include "storage/GameMetadataBackfill.h"

#include "config/Paths.h"
#include "core/GameIdentity.h"
#include "storage/GameIconCache.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
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

    // Every title candidate the detector logged is a way back to the row, not
    // just the Steam one: a non-Steam title (Xbox/MSIX, itch, standalone) logs
    // `steam: <none>`, and only accepting that field is why those games kept an
    // empty icon_path forever.
    //
    // The fields are not equally trustworthy, though, so a match carries the
    // rank of the field it came from and the whole log is scanned before
    // anything is written. A launcher shim gets logged under the game's window
    // title ("gamingservicesui.exe | window: Vampire Crawlers") one line BEFORE
    // the real executable, so first-match-wins would bind the shim's path.
    // `fromExe` outranks it: the executable named after the game IS the game.
    enum CandidateRank { RankFromExe = 0, RankProduct, RankSteam, RankWindow, RankCount };
    // Capture index per rank, matching the regex groups below.
    static const int kCaptureForRank[RankCount] = { 5, 3, 1, 2 };

    struct Best { QString path; int rank = RankCount; };
    QHash<int, Best> best;   // game id -> best evidence seen so far

    QTextStream in(&log);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const auto m = re.match(line);
        if (!m.hasMatch())
            continue;

        const QString path = m.captured(6).trimmed();
        if (path == QLatin1String("<none>") || !QFileInfo::exists(path))
            continue;

        for (int rank = 0; rank < RankCount; ++rank) {
            const QString name = m.captured(kCaptureForRank[rank]).trimmed();
            if (name.isEmpty() || name == QLatin1String("<none>"))
                continue;
            const QString key = GameIdentity::key(name);
            if (key.isEmpty())
                continue;

            for (const MissingGame& game : missing) {
                if (game.key != key)
                    continue;
                // FileDescription (capture 4) shares RankProduct with
                // ProductName; it is folded in by the loop below.
                Best& current = best[game.id];
                if (rank < current.rank)
                    current = { path, rank };
            }
        }

        // FileDescription is the same strength as ProductName but lives in its
        // own capture, so it gets a second pass at that rank.
        const QString description = m.captured(4).trimmed();
        if (!description.isEmpty() && description != QLatin1String("<none>")) {
            const QString key = GameIdentity::key(description);
            for (const MissingGame& game : missing) {
                if (game.key != key || key.isEmpty())
                    continue;
                Best& current = best[game.id];
                if (RankProduct < current.rank)
                    current = { path, RankProduct };
            }
        }
    }

    int updates = 0;
    for (const MissingGame& game : missing) {
        const auto it = best.constFind(game.id);
        if (it == best.constEnd() || it->path.isEmpty())
            continue;
        updateGameExecutable(db, game.id, it->path);
        qInfo() << "DB: backfilled game icon from historical detection for"
                << game.name << "->" << it->path;
        ++updates;
    }

    if (updates > 0)
        qInfo() << "DB: backfilled" << updates << "game icon(s) from gamehq.log";
}
