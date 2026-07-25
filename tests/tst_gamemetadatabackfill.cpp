#include "core/GameIdentity.h"
#include "storage/GameMetadataBackfill.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <QUuid>

// GameMetadataBackfill::selectBestPaths replays historical GameDetector log
// lines to recover an executable path for a game row that never got one (the
// case for every non-Steam launcher, which logs "steam: <none>"). These tests
// exercise the pure selection/ranking logic without touching a real database
// or a real gamehq.log.
class TestGameMetadataBackfill : public QObject
{
    Q_OBJECT

private:
    static QString line(const QString& steam, const QString& window, const QString& product,
                        const QString& fileDescription, const QString& fromExe,
                        const QString& path)
    {
        return QStringLiteral(
            "GameDetector title candidates for x.exe | pid: 1 | steam: %1 | window: %2 | "
            "ProductName: %3 | FileDescription: %4 | fromExe: %5 | path: %6")
            .arg(steam, window, product, fileDescription, fromExe, path);
    }

    static bool alwaysExists(const QString&) { return true; }

private slots:
    void fromExeOutranksWindowTitleRegardlessOfLineOrder()
    {
        const QString shimPath = QStringLiteral("C:/shim/gamingservicesui.exe");
        const QString realPath = QStringLiteral("C:/games/VampireCrawlers/game.exe");
        const GameMetadataBackfill::Target target{
            1, GameIdentity::key(QStringLiteral("Vampire Crawlers")) };

        // Shim logged one line ahead of the real executable: its own line
        // carries the game's title only as the *window*, not as fromExe.
        const QStringList inOrder{
            line("<none>", "Vampire Crawlers", "<none>", "<none>", "gamingservicesui", shimPath),
            line("<none>", "<none>", "<none>", "<none>", "Vampire Crawlers", realPath),
        };
        auto best = GameMetadataBackfill::selectBestPaths(inOrder, { target }, alwaysExists);
        QCOMPARE(best.value(1), realPath);

        // Order must not matter: rank, not first-match, decides the winner.
        const QStringList reversed{ inOrder[1], inOrder[0] };
        best = GameMetadataBackfill::selectBestPaths(reversed, { target }, alwaysExists);
        QCOMPARE(best.value(1), realPath);
    }

    void lowerRankNeverOverwritesHigherRank()
    {
        const QString goodPath = QStringLiteral("C:/games/Hades/Hades.exe");
        const QString shimPath = QStringLiteral("C:/shim/launcher.exe");
        const GameMetadataBackfill::Target target{ 7, GameIdentity::key(QStringLiteral("Hades")) };

        const QStringList lines{
            line("<none>", "<none>", "<none>", "<none>", "Hades", goodPath),
            // A later, weaker (window-only) candidate for the same game must
            // not displace the already-found fromExe match.
            line("<none>", "Hades", "<none>", "<none>", "launcher", shimPath),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QCOMPARE(best.value(7), goodPath);
    }

    void steamNameAloneStillMatches()
    {
        const QString path = QStringLiteral("C:/Steam/steamapps/common/Portal2/portal2.exe");
        const GameMetadataBackfill::Target target{ 3, GameIdentity::key(QStringLiteral("Portal 2")) };
        const QStringList lines{
            line("Portal 2", "<none>", "<none>", "<none>", "<none>", path),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QCOMPARE(best.value(3), path);
    }

    void fileDescriptionMatchesAtProductRank()
    {
        const QString path = QStringLiteral("C:/games/Foo/foo.exe");
        const GameMetadataBackfill::Target target{ 4, GameIdentity::key(QStringLiteral("Foo Bar")) };
        const QStringList lines{
            line("<none>", "<none>", "<none>", "Foo Bar", "<none>", path),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QCOMPARE(best.value(4), path);
    }

    void noneCandidateNeverMatches()
    {
        const GameMetadataBackfill::Target target{ 5, GameIdentity::key(QStringLiteral("Anything")) };
        const QStringList lines{
            line("<none>", "<none>", "<none>", "<none>", "<none>", "C:/games/Anything/x.exe"),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QVERIFY(!best.contains(5));
    }

    void pathThatNoLongerExistsIsSkipped()
    {
        const GameMetadataBackfill::Target target{ 6, GameIdentity::key(QStringLiteral("Gone")) };
        const QStringList lines{
            line("<none>", "<none>", "<none>", "<none>", "Gone", "C:/games/Gone/gone.exe"),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(
            lines, { target }, [](const QString&) { return false; });
        QVERIFY(!best.contains(6));
    }

    void unmatchedTargetGetsNoEntry()
    {
        // A game whose key never appears in the log (e.g. it already has both
        // executable_path and icon_path, so run() never even puts it in the
        // target list) must simply be absent from the result, not touched.
        const GameMetadataBackfill::Target target{
            9, GameIdentity::key(QStringLiteral("Never Logged")) };
        const QStringList lines{
            line("Something Else", "<none>", "<none>", "<none>", "<none>",
                 "C:/games/Other/other.exe"),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QVERIFY(best.isEmpty());
    }

    void fullyPopulatedGameIsNeverTouchedByRun()
    {
        // A game that already has both executable_path and icon_path (the
        // Steam-detected case, or any already-resolved title) must never even
        // enter run()'s candidate query, regardless of what the log says.
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QTemporaryDir exeDir;
        QVERIFY(exeDir.isValid());
        const QString missingExePath = exeDir.filePath(QStringLiteral("MissingGame.exe"));
        {
            QFile exe(missingExePath);
            QVERIFY(exe.open(QIODevice::WriteOnly));
            exe.write("stub");
        }

        const QString dbPath = tempDir.filePath(QStringLiteral("test.db"));
        const QString connectionName = QUuid::createUuid().toString();
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
            db.setDatabaseName(dbPath);
            QVERIFY(db.open());
            QSqlQuery create(db);
            QVERIFY2(create.exec(QStringLiteral(
                // last_seen_at is part of the real games table (CaptureDatabase
                // migration "last_seen_at") and run() writes it. Leaving it out
                // here made every UPDATE fail to prepare, which QSQLITE reports
                // as "Parameter count mismatch".
                "CREATE TABLE games ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, display_name TEXT NOT NULL, "
                "executable_path TEXT, icon_path TEXT, created_at TEXT NOT NULL, "
                "last_seen_at TEXT)")),
                qPrintable(create.lastError().text()));

            QSqlQuery insertComplete(db);
            insertComplete.prepare(QStringLiteral(
                "INSERT INTO games (display_name, executable_path, icon_path, created_at) "
                "VALUES ('Portal 2', 'C:/Steam/portal2.exe', 'C:/cache/portal2.png', 'now')"));
            QVERIFY2(insertComplete.exec(), qPrintable(insertComplete.lastError().text()));

            QSqlQuery insertMissing(db);
            insertMissing.prepare(QStringLiteral(
                "INSERT INTO games (display_name, executable_path, icon_path, created_at) "
                "VALUES ('Missing Game', NULL, NULL, 'now')"));
            QVERIFY2(insertMissing.exec(), qPrintable(insertMissing.lastError().text()));

            const QString logPath = tempDir.filePath(QStringLiteral("gamehq.log"));
            QFile logFile(logPath);
            QVERIFY(logFile.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream out(&logFile);
            // A log line that names the fully-populated game must be ignored
            // for it (it is never in the candidate query) even though it
            // would otherwise match; a second line resolves the actually
            // missing game.
            out << line("Portal 2", "<none>", "<none>", "<none>", "<none>",
                        "C:/somewhere/else/portal2-imposter.exe")
                << Qt::endl
                << line("<none>", "<none>", "<none>", "<none>", "Missing Game", missingExePath)
                << Qt::endl;
            logFile.close();

            GameMetadataBackfill::run(db, logPath);

            QSqlQuery verify(db);
            QVERIFY(verify.exec(QStringLiteral(
                "SELECT display_name, executable_path, icon_path FROM games ORDER BY id")));
            QVERIFY(verify.next());
            QCOMPARE(verify.value(0).toString(), QStringLiteral("Portal 2"));
            QCOMPARE(verify.value(1).toString(), QStringLiteral("C:/Steam/portal2.exe"));
            QCOMPARE(verify.value(2).toString(), QStringLiteral("C:/cache/portal2.png"));

            QVERIFY(verify.next());
            QCOMPARE(verify.value(0).toString(), QStringLiteral("Missing Game"));
            QCOMPARE(verify.value(1).toString(), missingExePath);
            QVERIFY(!verify.value(2).toString().isEmpty());
        }
        QSqlDatabase::removeDatabase(connectionName);
    }

    void malformedLinesAreIgnored()
    {
        const GameMetadataBackfill::Target target{
            2, GameIdentity::key(QStringLiteral("Whatever")) };
        const QStringList lines{
            QStringLiteral("not a detector line at all"),
            QStringLiteral("GameDetector title candidates for x.exe | incomplete"),
        };
        const auto best = GameMetadataBackfill::selectBestPaths(lines, { target }, alwaysExists);
        QVERIFY(best.isEmpty());
    }
};

// QApplication, not QTEST_GUILESS_MAIN: run() re-extracts icons through
// GameIconCache, whose QFileIconProvider fallback is a QtWidgets class that
// needs a GUI application object. See tst_captureiconrefresh.cpp.
QTEST_MAIN(TestGameMetadataBackfill)
#include "tst_gamemetadatabackfill.moc"
