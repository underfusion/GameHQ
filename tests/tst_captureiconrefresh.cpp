#include "storage/CaptureDatabase.h"
#include "storage/GameIconCache.h"

#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#include <QVariant>

// CaptureDatabase::refreshIconsForExtractorFormat re-extracts every game's
// icon once whenever GameIconCache's format version moves, so a library
// filled in by an older extractor recovers without a new capture. These
// tests exercise it end to end (real temp SQLite file, real dummy
// executables) without touching the real gamehq-data cache.
class TestCaptureIconRefresh : public QObject
{
    Q_OBJECT

private:
    // Opens a throwaway connection to the same file CaptureDatabase used, so
    // the test can inspect/mutate rows CaptureDatabase's own API cannot reach
    // (it only lists games that have captures).
    struct RawConnection
    {
        QString name;
        QSqlDatabase db;

        explicit RawConnection(const QString& path)
            : name(QUuid::createUuid().toString())
        {
            db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
            db.setDatabaseName(path);
        }
        ~RawConnection()
        {
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(name);
        }
    };

    static QString writeDummyExecutable(QTemporaryDir& dir, const QString& fileName)
    {
        const QString path = dir.filePath(fileName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return QString();
        file.write("stub-executable-bytes");
        file.close();
        return path;
    }

    static void setIconFormatSentinel(QSqlDatabase& db, const QString& value)
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO settings (key, value) VALUES ('internal.icon_format', :v) "
            "ON CONFLICT(key) DO UPDATE SET value = :v"));
        q.bindValue(QStringLiteral(":v"), value);
        QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
    }

    static int insertGameRow(QSqlDatabase& db, const QString& name, const QString& executablePath,
                             const QString& iconPath)
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO games (display_name, executable_path, icon_path, created_at) "
            "VALUES (:n, :e, :i, 'now')"));
        q.bindValue(QStringLiteral(":n"), name);
        q.bindValue(QStringLiteral(":e"), executablePath.isEmpty() ? QVariant() : QVariant(executablePath));
        q.bindValue(QStringLiteral(":i"), iconPath.isEmpty() ? QVariant() : QVariant(iconPath));
        if (!q.exec())
            return -1;
        return q.lastInsertId().toInt();
    }

    static QString iconPathFor(QSqlDatabase& db, int gameId)
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral("SELECT icon_path FROM games WHERE id = :id"));
        q.bindValue(QStringLiteral(":id"), gameId);
        if (!q.exec() || !q.next())
            return QString();
        return q.value(0).toString();
    }

private slots:
    void formatBumpReExtractsThenStaysIdempotent()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString dbPath = tempDir.filePath(QStringLiteral("test.db"));
        const QString exePath = writeDummyExecutable(tempDir, QStringLiteral("dummy.exe"));
        QVERIFY(!exePath.isEmpty());

        int gameId = -1;
        {
            CaptureDatabase db(dbPath);
            QVERIFY(db.open());
        }
        {
            RawConnection raw(dbPath);
            QVERIFY(raw.db.open());
            gameId = insertGameRow(raw.db, QStringLiteral("Dummy Game"), exePath,
                                   QStringLiteral("C:/stale/does-not-matter.png"));
            QVERIFY(gameId >= 0);
            setIconFormatSentinel(raw.db, QStringLiteral("v0-old-format"));
        }

        QString afterFirstRefresh;
        {
            CaptureDatabase db(dbPath);
            QVERIFY(db.open());
        }
        {
            RawConnection raw(dbPath);
            QVERIFY(raw.db.open());
            afterFirstRefresh = iconPathFor(raw.db, gameId);
        }
        // The stale path is a placeholder that was never written to disk, so
        // a real re-extraction must replace it with something that exists.
        QVERIFY(!afterFirstRefresh.isEmpty());
        QVERIFY(afterFirstRefresh != QStringLiteral("C:/stale/does-not-matter.png"));
        QVERIFY(QFileInfo::exists(afterFirstRefresh));

        // Re-opening with the sentinel now matching the current format must
        // not touch the row again.
        {
            CaptureDatabase db(dbPath);
            QVERIFY(db.open());
        }
        {
            RawConnection raw(dbPath);
            QVERIFY(raw.db.open());
            QCOMPARE(iconPathFor(raw.db, gameId), afterFirstRefresh);
        }
    }

    void missingExecutableNeverErasesAnExistingIcon()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString dbPath = tempDir.filePath(QStringLiteral("test.db"));
        const QString goodIcon = QStringLiteral("C:/cache/still-here.png");

        int gameId = -1;
        {
            CaptureDatabase db(dbPath);
            QVERIFY(db.open());
        }
        {
            RawConnection raw(dbPath);
            QVERIFY(raw.db.open());
            // The executable no longer exists (uninstalled/moved), but the
            // icon extracted while it did still should.
            gameId = insertGameRow(raw.db, QStringLiteral("Uninstalled Game"),
                                   QStringLiteral("C:/gone/uninstalled.exe"), goodIcon);
            QVERIFY(gameId >= 0);
            setIconFormatSentinel(raw.db, QStringLiteral("v0-old-format"));
        }
        {
            CaptureDatabase db(dbPath);
            QVERIFY(db.open());
        }
        {
            RawConnection raw(dbPath);
            QVERIFY(raw.db.open());
            QCOMPARE(iconPathFor(raw.db, gameId), goodIcon);
        }
    }
};

// QApplication, not QTEST_GUILESS_MAIN: the refresh path reaches
// GameIconCache::iconPathForExecutable, which falls back to QFileIconProvider.
// That is a QtWidgets class and dereferences the platform theme, so under a
// bare QCoreApplication it segfaults instead of returning a null icon. The app
// itself always runs as a QApplication, so this matches production.
QTEST_MAIN(TestCaptureIconRefresh)
#include "tst_captureiconrefresh.moc"
