#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include "TestReleaseSigner.h"
#include "updates/UpdateInstaller.h"
#include "updater/UpdaterTransaction.h"

namespace
{
// One staging directory holding a package plus the signed manifest and
// signature that authorise it, exactly as UpdateDownloader leaves them.
struct StagedRelease
{
    QString root;
    QString downloads;
    QString data;
    QString packagePath;
    VerifiedUpdate verified;
};

QString hexSha256(const QByteArray &contents)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(contents, QCryptographicHash::Sha256).toHex());
}

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

StagedRelease stageRelease(const QString &root, const QString &version,
                           const QByteArray &packageBytes, quint64 sequence = 5)
{
    StagedRelease staged;
    staged.root = root;
    staged.downloads = QDir(root).filePath(QStringLiteral(".update/downloads"));
    staged.data = QDir(root).filePath(QStringLiteral("gamehq-data"));
    QDir().mkpath(staged.downloads);
    QDir().mkpath(staged.data);

    const QString assetName = QStringLiteral("GameHQ-%1-win64-update.zip").arg(version);
    staged.packagePath = QDir(staged.downloads).filePath(assetName);
    if (!writeFile(staged.packagePath, packageBytes))
        return {};

    const QString packageHash = hexSha256(packageBytes);
    const auto manifest = test_release_signer::buildManifest(
        version.toStdString(), sequence,
        {{"update", assetName.toStdString(), static_cast<std::uint64_t>(packageBytes.size()),
          packageHash.toStdString()}});
    const std::string signature = test_release_signer::sign(manifest);

    const QString manifestPath = QDir(staged.downloads).filePath(QStringLiteral("gamehq-release.json"));
    const QString signaturePath = QDir(staged.downloads).filePath(QStringLiteral("gamehq-release.sig"));
    const QByteArray manifestBytes(reinterpret_cast<const char *>(manifest.data()),
                                   static_cast<qsizetype>(manifest.size()));
    if (!writeFile(manifestPath, manifestBytes)
        || !writeFile(signaturePath, QByteArray::fromStdString(signature) + "\n"))
        return {};

    staged.verified.packagePath = staged.packagePath;
    staged.verified.packageSha256 = QCryptographicHash::hash(packageBytes, QCryptographicHash::Sha256);
    staged.verified.version = version;
    staged.verified.manifestPath = manifestPath;
    staged.verified.signaturePath = signaturePath;
    staged.verified.manifestSha256 = hexSha256(manifestBytes);
    staged.verified.signature = QString::fromStdString(signature);
    staged.verified.keyId = QString::fromLatin1(test_release_signer::kTestKeyId);
    staged.verified.releaseSequence = sequence;
    staged.verified.artifactName = assetName;
    staged.verified.artifactSize = packageBytes.size();
    staged.verified.artifactSha256 = packageHash;
    return staged;
}
} // namespace

class UpdateInstallerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void writesHelperValidatedTransaction();
    void rejectsExternalOrChangedPackage();
    void rejectsUnsignedOrMismatchedManifest();
    void helperRejectsTamperedEvidence();
};

void UpdateInstallerTest::writesHelperValidatedTransaction()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-XXXXXX")));
    QVERIFY(dir.isValid());
    const StagedRelease staged = stageRelease(dir.path(), QStringLiteral("1.2.3"),
                                              QByteArrayLiteral("verified package"));
    QVERIFY(!staged.root.isEmpty());

    QString transactionPath;
    QString error;
    QVERIFY2(UpdateInstaller::prepareTransaction(dir.path(), staged.data, staged.verified,
                                                 transactionPath, error), qPrintable(error));

    updater::Transaction tx;
    std::string helperError;
    QVERIFY2(updater::loadAndValidateTransaction(transactionPath.toStdWString(), tx, helperError),
             helperError.c_str());
    QCOMPARE(QString::fromStdString(tx.expectedVersion), QStringLiteral("1.2.3"));
    QCOMPARE(QString::fromStdWString(tx.dataDir.wstring()), QDir::toNativeSeparators(staged.data));
    // The signed evidence must survive the round trip.
    QCOMPARE(QString::fromStdString(tx.releaseKeyId), staged.verified.keyId);
    QCOMPARE(tx.releaseSequence, staged.verified.releaseSequence);
    QCOMPARE(QString::fromStdString(tx.artifactSha256), staged.verified.artifactSha256);
    QCOMPARE(tx.artifactSize, staged.verified.artifactSize);
    QVERIFY(updater::verifyReleaseAuthorisation(tx, helperError));
}

void UpdateInstallerTest::rejectsExternalOrChangedPackage()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-reject-XXXXXX")));
    QVERIFY(dir.isValid());
    const StagedRelease staged = stageRelease(dir.path(), QStringLiteral("1.2.3"),
                                              QByteArrayLiteral("bytes"));
    QVERIFY(!staged.root.isEmpty());
    QString transactionPath;
    QString error;

    // A package outside the staging directory is never installable.
    const QString outside = QDir(dir.path()).filePath(QStringLiteral("outside.zip"));
    QVERIFY(writeFile(outside, QByteArrayLiteral("bytes")));
    VerifiedUpdate external = staged.verified;
    external.packagePath = outside;
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, external,
                                                 transactionPath, error));

    // The staged bytes changed after verification.
    VerifiedUpdate changed = staged.verified;
    changed.packageSha256 = QByteArray(32, '\0');
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, changed,
                                                 transactionPath, error));
    QVERIFY(error.contains(QStringLiteral("changed")));

    // Incomplete evidence is refused before anything is written.
    VerifiedUpdate incomplete = staged.verified;
    incomplete.keyId.clear();
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, incomplete,
                                                 transactionPath, error));
}

void UpdateInstallerTest::rejectsUnsignedOrMismatchedManifest()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-manifest-XXXXXX")));
    QVERIFY(dir.isValid());
    const StagedRelease staged = stageRelease(dir.path(), QStringLiteral("1.2.3"),
                                              QByteArrayLiteral("payload"));
    QVERIFY(!staged.root.isEmpty());
    QString transactionPath;
    QString error;

    // A manifest that lives outside the staging directory cannot be evidence.
    const QString foreignManifest = QDir(dir.path()).filePath(QStringLiteral("gamehq-release.json"));
    QVERIFY(QFile::copy(staged.verified.manifestPath, foreignManifest));
    VerifiedUpdate outsideManifest = staged.verified;
    outsideManifest.manifestPath = foreignManifest;
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, outsideManifest,
                                                 transactionPath, error));

    // Artifact metadata that disagrees with the file on disk.
    VerifiedUpdate wrongSize = staged.verified;
    wrongSize.artifactSize = staged.verified.artifactSize + 1;
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, wrongSize,
                                                 transactionPath, error));

    VerifiedUpdate wrongName = staged.verified;
    wrongName.artifactName = QStringLiteral("GameHQ-9.9.9-win64-update.zip");
    QVERIFY(!UpdateInstaller::prepareTransaction(dir.path(), staged.data, wrongName,
                                                 transactionPath, error));
}

void UpdateInstallerTest::helperRejectsTamperedEvidence()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-installer-helper-XXXXXX")));
    QVERIFY(dir.isValid());
    const StagedRelease staged = stageRelease(dir.path(), QStringLiteral("1.2.3"),
                                              QByteArrayLiteral("real payload"));
    QVERIFY(!staged.root.isEmpty());
    QString transactionPath;
    QString error;
    QVERIFY2(UpdateInstaller::prepareTransaction(dir.path(), staged.data, staged.verified,
                                                 transactionPath, error), qPrintable(error));

    updater::Transaction tx;
    std::string helperError;
    QVERIFY(updater::loadAndValidateTransaction(transactionPath.toStdWString(), tx, helperError));

    // Swapping the archive after the transaction was written must be caught by
    // the helper's own re-verification, not only by the app.
    QVERIFY(writeFile(staged.packagePath, QByteArrayLiteral("attacker payload")));
    QVERIFY(!updater::verifyReleaseAuthorisation(tx, helperError));

    // Flipping one manifest byte breaks the signature.
    QVERIFY(writeFile(staged.packagePath, QByteArrayLiteral("real payload")));
    QFile manifest(staged.verified.manifestPath);
    QVERIFY(manifest.open(QIODevice::ReadOnly));
    QByteArray bytes = manifest.readAll();
    manifest.close();
    QByteArray tampered = bytes;
    tampered[10] = tampered[10] ^ 0x01;
    QVERIFY(writeFile(staged.verified.manifestPath, tampered));
    QVERIFY(!updater::verifyReleaseAuthorisation(tx, helperError));

    // Replacing the signature with a well-formed but wrong one is rejected too.
    QVERIFY(writeFile(staged.verified.manifestPath, bytes));
    QVERIFY(updater::verifyReleaseAuthorisation(tx, helperError));
    const auto otherManifest = test_release_signer::buildManifest(
        "1.2.3", 6,
        {{"update", "GameHQ-1.2.3-win64-update.zip", 12, std::string(64, 'a')}});
    QVERIFY(writeFile(staged.verified.signaturePath,
                      QByteArray::fromStdString(test_release_signer::sign(otherManifest))));
    QVERIFY(!updater::verifyReleaseAuthorisation(tx, helperError));
}

QTEST_GUILESS_MAIN(UpdateInstallerTest)
#include "tst_updateinstaller.moc"
