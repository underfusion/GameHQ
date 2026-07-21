#include "app/ReleaseNotes.h"

#include <QTest>

class ReleaseNotesTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesStructuredPlainText();
    void rejectsNonStringItems();
    void rejectsInvalidVersion();
    void rejectsOversizedDocuments();
};

void ReleaseNotesTest::parsesStructuredPlainText()
{
    const ReleaseNotes notes = ReleaseNotes::fromJson(R"({
        "version":"1.2.3",
        "sections":[{"title":"Added","items":["First item","Second item"]}]
    })");

    QVERIFY(notes.isValid());
    QCOMPARE(notes.version(), QStringLiteral("1.2.3"));
    QCOMPARE(notes.sections().size(), 1);
    const QVariantMap section = notes.sections().first().toMap();
    QCOMPARE(section.value(QStringLiteral("title")).toString(), QStringLiteral("Added"));
    QCOMPARE(section.value(QStringLiteral("items")).toStringList().size(), 2);
}

void ReleaseNotesTest::rejectsNonStringItems()
{
    const ReleaseNotes notes = ReleaseNotes::fromJson(R"({
        "version":"1.2.3",
        "sections":[{"title":"Added","items":[{"html":"<img src='file:///x'>"}]}]
    })");
    QVERIFY(!notes.isValid());
}

void ReleaseNotesTest::rejectsInvalidVersion()
{
    const ReleaseNotes notes = ReleaseNotes::fromJson(R"({
        "version":"v1.2.3-beta",
        "sections":[{"title":"Added","items":["Item"]}]
    })");
    QVERIFY(!notes.isValid());
}

void ReleaseNotesTest::rejectsOversizedDocuments()
{
    QByteArray json = R"({"version":"1.2.3","sections":[{"title":"Added","items":[)";
    for (int i = 0; i < 21; ++i) {
        if (i > 0)
            json += ',';
        json += R"("Item")";
    }
    json += R"(]}]})";
    QVERIFY(!ReleaseNotes::fromJson(json).isValid());
}

QTEST_APPLESS_MAIN(ReleaseNotesTest)
#include "tst_releasenotes.moc"
