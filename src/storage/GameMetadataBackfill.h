#pragma once

#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class GameMetadataBackfill
{
public:
    // logFilePath overrides the real gamehq.log location; only tests should pass one.
    static void run(QSqlDatabase& db, const QString& logFilePath = QString());

    // One game row this backfill can still match against: db id plus its
    // GameIdentity::key(display_name). Exposed for testing.
    struct Target
    {
        int id;
        QString key;
    };

    // Scans historical `GameDetector title candidates for ...` log lines and
    // returns, for each target matched by at least one logged candidate, the
    // highest-ranked executable path found. Ranking (best first): the exe
    // name itself (fromExe), then ProductName/FileDescription, then the
    // Steam name, then the window title — a launcher shim can log the game's
    // window title one line ahead of the real executable, so first-match-wins
    // would bind the shim's path instead. pathExists is injected so callers
    // (and tests) don't depend on the real filesystem.
    static QHash<int, QString> selectBestPaths(
        const QStringList& logLines, const QVector<Target>& targets,
        const std::function<bool(const QString&)>& pathExists);
};
