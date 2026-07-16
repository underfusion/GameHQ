#include "core/GameIdentity.h"

#include <QTest>

// GameIdentity turns a game's display name into a filesystem-safe folder name
// and a comparison key, and reads a game back out of a capture's path. All of
// it is pure string/path arithmetic, so it is testable as-is.
class TestGameIdentity : public QObject
{
    Q_OBJECT

private slots:
    void folderName_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");

        QTest::newRow("plain") << "Hades" << "Hades";
        // Every character Windows forbids in a path segment becomes '_'.
        QTest::newRow("colon") << "Marvel's Spider-Man: Miles Morales"
                               << "Marvel's Spider-Man_ Miles Morales";
        QTest::newRow("slash") << "Ratchet/Clank" << "Ratchet_Clank";
        QTest::newRow("backslash") << "A\\B" << "A_B";
        QTest::newRow("all forbidden") << "<>:\"/\\|?*" << "_________";
        QTest::newRow("trimmed") << "  Hades  " << "Hades";
        // An empty or whitespace-only name would produce an unusable folder.
        QTest::newRow("empty") << "" << "Unknown Game";
        QTest::newRow("whitespace only") << "   " << "Unknown Game";
    }

    void folderName()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(GameIdentity::folderName(input), expected);
    }

    void key_foldsCaseAndWhitespace()
    {
        // The key exists so two spellings of one game collapse to one row.
        QCOMPARE(GameIdentity::key("HADES"), GameIdentity::key("hades"));
        QCOMPARE(GameIdentity::key("Hades   II"), GameIdentity::key("Hades II"));
        QCOMPARE(GameIdentity::key("  Hades  "), GameIdentity::key("Hades"));
    }

    void key_appliesFolderSanitizingFirst()
    {
        // A raw title and its sanitized folder name must land on the same key,
        // otherwise a rescan would duplicate the game row.
        QCOMPARE(GameIdentity::key("Ratchet/Clank"), GameIdentity::key("Ratchet_Clank"));
    }

    void key_keepsDistinctGamesDistinct()
    {
        QVERIFY(GameIdentity::key("Hades") != GameIdentity::key("Hades II"));
    }

    void hasFolderForbiddenChar_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("expected");

        QTest::newRow("clean") << "Hades" << false;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("colon") << "Halo: Reach" << true;
        QTest::newRow("question") << "Who?" << true;
        QTest::newRow("apostrophe is allowed") << "Marvel's" << false;
    }

    void hasFolderForbiddenChar()
    {
        QFETCH(QString, input);
        QFETCH(bool, expected);
        QCOMPARE(GameIdentity::hasFolderForbiddenChar(input), expected);
    }

    void inferFromPath_data()
    {
        QTest::addColumn<QString>("root");
        QTest::addColumn<QString>("filePath");
        QTest::addColumn<QString>("expected");

        QTest::newRow("screenshots")
            << "C:/Captures" << "C:/Captures/Hades/Screenshots/shot.png" << "Hades";
        QTest::newRow("clips")
            << "C:/Captures" << "C:/Captures/Hades/Clips/clip.mp4" << "Hades";
        QTest::newRow("directly in game folder")
            << "C:/Captures" << "C:/Captures/Hades/shot.png" << "Hades";
        QTest::newRow("deeper nesting still reads the game folder")
            << "C:/Captures" << "C:/Captures/Hades/Screenshots/2026/shot.png" << "Hades";
        // No game folder between root and file → nothing to infer.
        QTest::newRow("loose file in root")
            << "C:/Captures" << "C:/Captures/shot.png" << "Unknown Game";
        QTest::newRow("trailing slash on root")
            << "C:/Captures/" << "C:/Captures/Hades/Screenshots/shot.png" << "Hades";
        QTest::newRow("game name with spaces")
            << "C:/Captures" << "C:/Captures/Hades II/Clips/clip.mp4" << "Hades II";
    }

    void inferFromPath()
    {
        QFETCH(QString, root);
        QFETCH(QString, filePath);
        QFETCH(QString, expected);
        QCOMPARE(GameIdentity::inferFromPath(root, filePath), expected);
    }
};

QTEST_APPLESS_MAIN(TestGameIdentity)
#include "tst_gameidentity.moc"
