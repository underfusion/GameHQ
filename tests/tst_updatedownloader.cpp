#include "updates/UpdateDownloader.h"
#include "updates/ReleaseInfo.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class UpdateDownloaderTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void parsesCommonChecksumFormats();
    void rejectsMalformedOrMismatchedChecksum();
    void rejectsCorruptedPackage();
    void detectsIncompleteReleaseAssets();
};

void UpdateDownloaderTest::detectsIncompleteReleaseAssets()
{
    ReleaseInfo release;
    release.version = QStringLiteral("1.2.3");
    QVERIFY(!release.hasCompleteUpdateAssets());
    release.zipName = QStringLiteral("GameHQ-1.2.3-win64-update.zip");
    release.zipUrl = QStringLiteral("https://example.invalid/update.zip");
    release.zipSize = 1024;
    QVERIFY(!release.hasCompleteUpdateAssets()); // checksum still uploading
    release.checksumUrl = QStringLiteral("https://example.invalid/update.zip.sha256");
    QVERIFY(release.hasCompleteUpdateAssets());
    release.zipSize = 0;
    QVERIFY(!release.hasCompleteUpdateAssets());
}

void UpdateDownloaderTest::parsesCommonChecksumFormats()
{
    const QByteArray digest(32, '\x5a');
    const QByteArray hex = digest.toHex();
    QByteArray parsed;
    QString error;

    QVERIFY(UpdateDownloader::parseChecksum(hex + "\n", "package.zip", parsed, error));
    QCOMPARE(parsed, digest);
    QVERIFY(UpdateDownloader::parseChecksum(hex.toUpper() + "  *package.zip\r\n",
                                             "package.zip", parsed, error));
    QCOMPARE(parsed, digest);
}

void UpdateDownloaderTest::rejectsMalformedOrMismatchedChecksum()
{
    const QByteArray hex = QByteArray(32, '\x01').toHex();
    QByteArray parsed;
    QString error;

    QVERIFY(!UpdateDownloader::parseChecksum("not-a-hash", "package.zip", parsed, error));
    QVERIFY(!UpdateDownloader::parseChecksum(hex + "  other.zip\n",
                                              "package.zip", parsed, error));
    QVERIFY(!UpdateDownloader::parseChecksum(hex + "\n" + hex + "\n",
                                              "package.zip", parsed, error));
}

void UpdateDownloaderTest::rejectsCorruptedPackage()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updatedownloader-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("package.zip"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("original package"), qint64(16));
    file.close();

    const QByteArray expected = QCryptographicHash::hash("different package",
                                                         QCryptographicHash::Sha256);
    QByteArray actual;
    QString error;
    QVERIFY(!UpdateDownloader::verifyFile(path, expected, actual, error));
    QVERIFY(error.contains(QStringLiteral("rejected")));
    QVERIFY(actual != expected);
}

QTEST_GUILESS_MAIN(UpdateDownloaderTest)
#include "tst_updatedownloader.moc"
