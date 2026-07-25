#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include "updates/UpdatePreflight.h"

class UpdatePreflightTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void acceptsPackagedInstalledAndPortableLayouts_data();
    void acceptsPackagedInstalledAndPortableLayouts();
    void rejectsIncompleteLayoutAndActiveTransaction();
};

void UpdatePreflightTest::acceptsPackagedInstalledAndPortableLayouts_data()
{
    QTest::addColumn<bool>("portable");
    QTest::newRow("installed") << false;
    QTest::newRow("portable") << true;
}

void UpdatePreflightTest::acceptsPackagedInstalledAndPortableLayouts()
{
    QFETCH(bool, portable);
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-preflight-XXXXXX")));
    QVERIFY(dir.isValid());
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral("app")));
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile file(path); return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe")), "launcher"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("GameHQUpdater.exe")), "helper"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe")), "app"));
    if (portable)
        QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("portable.flag")), "choice"));
    QString error;
    QVERIFY2(UpdatePreflight::check(dir.path(), 1024, error), qPrintable(error));
    const QString marker = QDir(dir.path()).filePath(QStringLiteral("portable.flag"));
    QCOMPARE(QFileInfo::exists(marker), portable);
    if (portable) {
        QFile file(marker); QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("choice"));
    }
}

void UpdatePreflightTest::rejectsIncompleteLayoutAndActiveTransaction()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-preflight-reject-XXXXXX")));
    QVERIFY(dir.isValid());
    QString error;
    QVERIFY(!UpdatePreflight::check(dir.path(), 1024, error));
    QVERIFY(error.contains(QStringLiteral("packaged")));
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral("app")));
    auto touch = [](const QString &path) { QFile file(path); return file.open(QIODevice::WriteOnly); };
    QVERIFY(touch(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe"))));
    QVERIFY(touch(QDir(dir.path()).filePath(QStringLiteral("GameHQUpdater.exe"))));
    QVERIFY(touch(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe"))));
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral(".update")));
    QVERIFY(touch(QDir(dir.path()).filePath(QStringLiteral(".update/transaction.phase"))));
    QVERIFY(!UpdatePreflight::check(dir.path(), 1024, error));
    QVERIFY(error.contains(QStringLiteral("previous update")));
}

QTEST_GUILESS_MAIN(UpdatePreflightTest)
#include "tst_updatepreflight.moc"
