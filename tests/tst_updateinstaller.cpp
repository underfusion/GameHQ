#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include "updates/UpdateInstaller.h"
#include "updater/UpdaterTransaction.h"

class UpdateInstallerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void writesHelperValidatedTransaction();
    void rejectsExternalOrChangedPackage();
};

void UpdateInstallerTest::writesHelperValidatedTransaction()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString downloads = QDir(dir.path()).filePath(QStringLiteral(".update/downloads"));
    const QString data = QDir(dir.path()).filePath(QStringLiteral("gamehq-data"));
    QDir().mkpath(downloads); QDir().mkpath(data);
    const QString package = QDir(downloads).filePath(QStringLiteral("GameHQ-1.2.3-win64-update.zip"));
    QFile file(package); QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("verified package") == 16); file.close();
    const QByteArray digest = QCryptographicHash::hash("verified package", QCryptographicHash::Sha256);
    QString transactionPath, error;
    QVERIFY2(UpdateInstaller::prepareTransaction(dir.path(), data, package,
                                                  QStringLiteral("1.2.3"), digest,
                                                  transactionPath, error), qPrintable(error));
    updater::Transaction tx;
    std::string helperError;
    QVERIFY2(updater::loadAndValidateTransaction(transactionPath.toStdWString(), tx, helperError),
             helperError.c_str());
    QCOMPARE(QString::fromStdString(tx.expectedVersion), QStringLiteral("1.2.3"));
    QCOMPARE(QString::fromStdWString(tx.dataDir.wstring()), QDir::toNativeSeparators(data));
}

void UpdateInstallerTest::rejectsExternalOrChangedPackage()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-reject-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString data = QDir(dir.path()).filePath(QStringLiteral("gamehq-data"));
    QDir().mkpath(data);
    const QString outside = QDir(dir.path()).filePath(QStringLiteral("outside.zip"));
    QFile file(outside); QVERIFY(file.open(QIODevice::WriteOnly)); QVERIFY(file.write("bytes") == 5); file.close();
    const QByteArray digest = QCryptographicHash::hash("bytes", QCryptographicHash::Sha256).toHex();
    QString transactionPath, error;
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), data, outside,
                                                  QStringLiteral("1.2.3"), digest,
                                                  transactionPath, error));
    const QString downloads = QDir(dir.path()).filePath(QStringLiteral(".update/downloads"));
    QDir().mkpath(downloads);
    const QString package = QDir(downloads).filePath(QStringLiteral("GameHQ-1.2.3-win64-update.zip"));
    QVERIFY(QFile::copy(outside, package));
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), data, package,
                                                  QStringLiteral("1.2.3"), QByteArray(32, '\0'),
                                                  transactionPath, error));
    QVERIFY(error.contains(QStringLiteral("changed")));
}

QTEST_GUILESS_MAIN(UpdateInstallerTest)
#include "tst_updateinstaller.moc"
