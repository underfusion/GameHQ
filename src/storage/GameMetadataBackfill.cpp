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

const QRegularExpression& detectorLineRegex()
{
    static const QRegularExpression re(
        QStringLiteral(R"(GameDetector title candidates for .* \| steam: ([^|]+) \| window: ([^|]+) \| ProductName: ([^|]+) \| FileDescription: ([^|]+) \| fromExe: ([^|]+) \| path: (.+)$)"));
    return re;
}

// Capture index per rank (RankFromExe, RankProduct, RankSteam, RankWindow).
constexpr int kCaptureForRank[] = { 5, 3, 1, 2 };
constexpr int kRankCount = 4;
constexpr int kRankProduct = 1;
} // namespace

QHash<int, QString> GameMetadataBackfill::selectBestPaths(
    const QStringList& logLines, const QVector<Target>& targets,
    const std::function<bool(const QString&)>& pathExists)
{
    struct Best { QString path; int rank = kRankCount; };
    QHash<int, Best> best;   // game id -> best evidence seen so far
    const QRegularExpression& re = detectorLineRegex();

    for (const QString& line : logLines) {
        const auto m = re.match(line);
        if (!m.hasMatch())
            continue;

        const QString path = m.captured(6).trimmed();
        if (path == QLatin1String("<none>") || !pathExists(path))
            continue;

        for (int rank = 0; rank < kRankCount; ++rank) {
            const QString name = m.captured(kCaptureForRank[rank]).trimmed();
            if (name.isEmpty() || name == QLatin1String("<none>"))
                continue;
            const QString key = GameIdentity::key(name);
            if (key.isEmpty())
                continue;

            for (const Target& target : targets) {
                if (target.key != key)
                    continue;
                Best& current = best[target.id];
                if (rank < current.rank)
                    current = { path, rank };
            }
        }

        // FileDescription is the same strength as ProductName but lives in its
        // own capture, so it gets a second pass at that rank.
        const QString description = m.captured(4).trimmed();
        if (!description.isEmpty() && description != QLatin1String("<none>")) {
            const QString key = GameIdentity::key(description);
            if (!key.isEmpty()) {
                for (const Target& target : targets) {
                    if (target.key != key)
                        continue;
                    Best& current = best[target.id];
                    if (kRankProduct < current.rank)
                        current = { path, kRankProduct };
                }
            }
        }
    }

    QHash<int, QString> result;
    for (auto it = best.constBegin(); it != best.constEnd(); ++it) {
        if (!it.value().path.isEmpty())
            result.insert(it.key(), it.value().path);
    }
    return result;
}

void GameMetadataBackfill::run(QSqlDatabase& db, const QString& logFilePath)
{
    QFile log(logFilePath.isEmpty() ? Paths::logsDir() + QStringLiteral("/gamehq.log") : logFilePath);
    if (!log.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QVector<Target> missing;
    struct MissingGame { int id; QString name; };
    QVector<MissingGame> missingNames;
    QSqlQuery games(db);
    games.prepare(QStringLiteral(
        "SELECT id, display_name FROM games "
        "WHERE (executable_path IS NULL OR executable_path = '') "
        "   OR (icon_path IS NULL OR icon_path = '')"));
    if (!games.exec())
        return;
    while (games.next()) {
        const int id = games.value(0).toInt();
        const QString name = games.value(1).toString();
        missing.append({ id, GameIdentity::key(name) });
        missingNames.append({ id, name });
    }
    if (missing.isEmpty())
        return;

    QStringList lines;
    QTextStream in(&log);
    while (!in.atEnd())
        lines.append(in.readLine());

    const QHash<int, QString> best = selectBestPaths(
        lines, missing, [](const QString& path) { return QFileInfo::exists(path); });

    int updates = 0;
    for (const MissingGame& game : missingNames) {
        const auto it = best.constFind(game.id);
        if (it == best.constEnd())
            continue;
        updateGameExecutable(db, game.id, it.value());
        qInfo() << "DB: backfilled game icon from historical detection for"
                << game.name << "->" << it.value();
        ++updates;
    }

    if (updates > 0)
        qInfo() << "DB: backfilled" << updates << "game icon(s) from gamehq.log";
}
