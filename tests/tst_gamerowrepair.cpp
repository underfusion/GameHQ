#include "storage/GameRowRepair.h"

#include <QTest>

// isBetterDisplayName decides which of two spellings of the same game wins when
// duplicate rows are merged. The rule is deliberately not "longest wins": a name
// that still carries characters the filesystem forbids is the *unsanitized* one,
// i.e. the real title as the game reports it, and it beats a folder-derived name
// no matter the lengths.
class TestGameRowRepair : public QObject
{
    Q_OBJECT

private slots:
    void anythingBeatsEmpty()
    {
        QVERIFY(GameRowRepair::isBetterDisplayName("Hades", ""));
        // Even a single character — an empty display name is never worth keeping.
        QVERIFY(GameRowRepair::isBetterDisplayName("H", ""));
    }

    void unsanitizedNameWins()
    {
        // "Halo: Reach" kept its colon, so it came from the game, not a folder.
        QVERIFY(GameRowRepair::isBetterDisplayName("Halo: Reach", "Halo_ Reach"));
    }

    void unsanitizedNameWinsEvenWhenShorter()
    {
        // The forbidden-char check runs before the length comparison, so the
        // shorter-but-real name still wins.
        QVERIFY(GameRowRepair::isBetterDisplayName("A:B", "A_B_Long_Folder_Name"));
    }

    void sanitizedNameNeverReplacesUnsanitized()
    {
        QVERIFY(!GameRowRepair::isBetterDisplayName("Halo_ Reach", "Halo: Reach"));
        QVERIFY(!GameRowRepair::isBetterDisplayName("A_B_Long_Folder_Name", "A:B"));
    }

    void longerWinsWhenBothAreClean()
    {
        QVERIFY(GameRowRepair::isBetterDisplayName("Hades II", "Hades"));
        QVERIFY(!GameRowRepair::isBetterDisplayName("Hades", "Hades II"));
    }

    void longerWinsWhenBothAreUnsanitized()
    {
        QVERIFY(GameRowRepair::isBetterDisplayName("Halo: Reach Remastered", "Halo: Reach"));
    }

    void equalNamesAreNotAnImprovement()
    {
        // Guards the merge loop against churning on a tie.
        QVERIFY(!GameRowRepair::isBetterDisplayName("Hades", "Hades"));
        QVERIFY(!GameRowRepair::isBetterDisplayName("Halo: Reach", "Halo: Reach"));
    }

    void sameLengthIsNotAnImprovement()
    {
        QVERIFY(!GameRowRepair::isBetterDisplayName("Hades", "Bogus"));
    }
};

QTEST_APPLESS_MAIN(TestGameRowRepair)
#include "tst_gamerowrepair.moc"
