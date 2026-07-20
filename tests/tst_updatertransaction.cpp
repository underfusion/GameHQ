#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>
#include <miniz.h>
#include "updater/UpdaterDataSnapshot.h"
#include "updater/UpdaterSwap.h"
#include "updater/UpdaterHealth.h"
#include "updater/UpdaterRecovery.h"
#include "launcher/UpdaterPromotion.h"
#include "core/UpdateMaintenance.h"
#include <windows.h>

class UpdaterTransactionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void dryRunListsOperationsWithoutWriting();
    void rejectsPathOutsidePackageRoot();
    void stagesValidPackage();
    void rejectsTraversalAndRemovesStaging();
    void rejectsForbiddenDataAndBadManifest();
    void rejectsPackageChangedAfterVerification();
    void snapshotsAndRestoresDataWithoutTouchingCaptures();
    void swapsOnlyOwnedProgramFiles();
    void lockedFileAbortsAndRollsBack();
    void healthyStartPublishesToken();
    void missingHealthTokenTimesOut();
    void failedHealthRollbackRestoresProgramAndData();
    void interruptedMixedSwapRecoversPreviousProgram();
    void rejectsPackageRequiringNewerUpdater();
    void promotesOnlySelfTestingPendingHelper();
    void completeApplyCleansStaleStagingAndPreservesUserData();

private:
    static QString writeFixture(const QString &root, const QString &backupDir);
    static bool writeZip(const QString &path, const QList<QPair<QByteArray, QByteArray>> &entries);
    static bool syncTransactionHash(const QString &transaction, const QString &package);
    static QProcess *runHelper(const QString &mode, const QString &transaction, QObject *parent);
};

QString UpdaterTransactionTest::writeFixture(const QString &root, const QString &backupDir)
{
    const QString updateDir = QDir(root).filePath(QStringLiteral(".update"));
    const QString downloadsDir = QDir(updateDir).filePath(QStringLiteral("downloads"));
    QDir().mkpath(downloadsDir);
    QDir().mkpath(QDir(root).filePath(QStringLiteral("gamehq-data")));
    const QString packagePath = QDir(downloadsDir).filePath(
        QStringLiteral("GameHQ-9.8.7-win64-update.zip"));
    QFile package(packagePath);
    if (!package.open(QIODevice::WriteOnly) || package.write("verified package") != 16)
        return {};
    package.close();

    const QByteArray sha = QCryptographicHash::hash("verified package", QCryptographicHash::Sha256).toHex();
    const QJsonObject object {
        { QStringLiteral("schemaVersion"), 1 },
        { QStringLiteral("productId"), QStringLiteral("underfusion.gamehq") },
        { QStringLiteral("expectedVersion"), QStringLiteral("9.8.7") },
        { QStringLiteral("expectedSha256"), QString::fromLatin1(sha) },
        { QStringLiteral("packageRoot"), QDir::toNativeSeparators(root) },
        { QStringLiteral("packagePath"), QDir::toNativeSeparators(packagePath) },
        { QStringLiteral("stagingDir"), QDir::toNativeSeparators(QDir(updateDir).filePath(QStringLiteral("staging"))) },
        { QStringLiteral("backupDir"), QDir::toNativeSeparators(backupDir) },
        { QStringLiteral("restartExecutable"), QDir::toNativeSeparators(QDir(root).filePath(QStringLiteral("GameHQ.exe"))) },
        { QStringLiteral("healthTokenPath"), QDir::toNativeSeparators(QDir(updateDir).filePath(QStringLiteral("healthy.token"))) },
        { QStringLiteral("dataDir"), QDir::toNativeSeparators(QDir(root).filePath(QStringLiteral("gamehq-data"))) },
        { QStringLiteral("dataSnapshotDir"), QDir::toNativeSeparators(QDir(updateDir).filePath(QStringLiteral("data-snapshot"))) },
        { QStringLiteral("phase"), QStringLiteral("download_verified") }
    };
    const QString transactionPath = QDir(updateDir).filePath(QStringLiteral("transaction.json"));
    QFile transaction(transactionPath);
    if (!transaction.open(QIODevice::WriteOnly)
        || transaction.write(QJsonDocument(object).toJson(QJsonDocument::Compact)) <= 0)
        return {};
    return transactionPath;
}

bool UpdaterTransactionTest::writeZip(
    const QString &path, const QList<QPair<QByteArray, QByteArray>> &entries)
{
    mz_zip_archive zip{};
    const QByteArray nativePath = QFile::encodeName(path);
    if (!mz_zip_writer_init_file(&zip, nativePath.constData(), 0))
        return false;
    bool ok = true;
    for (const auto &entry : entries) {
        if (!mz_zip_writer_add_mem(&zip, entry.first.constData(), entry.second.constData(),
                                   static_cast<size_t>(entry.second.size()), MZ_BEST_SPEED)) {
            ok = false;
            break;
        }
    }
    if (ok)
        ok = mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    return ok;
}

QProcess *UpdaterTransactionTest::runHelper(const QString &mode, const QString &transaction,
                                            QObject *parent)
{
    auto *process = new QProcess(parent);
    process->start(QStringLiteral(UPDATER_EXE), { mode, transaction });
    return process;
}

bool UpdaterTransactionTest::syncTransactionHash(const QString &transaction, const QString &package)
{
    QFile packageFile(package);
    if (!packageFile.open(QIODevice::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&packageFile))
        return false;
    QFile transactionFile(transaction);
    if (!transactionFile.open(QIODevice::ReadOnly))
        return false;
    QJsonObject object = QJsonDocument::fromJson(transactionFile.readAll()).object();
    transactionFile.close();
    object.insert(QStringLiteral("expectedSha256"), QString::fromLatin1(hash.result().toHex()));
    if (!transactionFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return transactionFile.write(QJsonDocument(object).toJson(QJsonDocument::Compact)) > 0;
}

void UpdaterTransactionTest::dryRunListsOperationsWithoutWriting()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-dryrun-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString staging = QDir(dir.path()).filePath(QStringLiteral(".update/staging"));
    const QString backup = QDir(dir.path()).filePath(QStringLiteral(".update/backup"));
    const QString transaction = writeFixture(dir.path(), backup);
    QVERIFY(!transaction.isEmpty());
    QVERIFY(!QFileInfo::exists(staging));
    QVERIFY(!QFileInfo::exists(backup));

    QProcess process;
    process.start(QStringLiteral(UPDATER_EXE), { QStringLiteral("--dry-run"), transaction });
    QVERIFY(process.waitForFinished(10000));
    QCOMPARE(process.exitCode(), 0);
    const QByteArray output = process.readAllStandardOutput();
    QVERIFY(output.contains("DRY RUN - no files will be changed"));
    QVERIFY(output.contains("BACKUP IF PRESENT GameHQ.exe"));
    QVERIFY(output.contains("INSTALL IF PRESENT app/"));
    QVERIFY(output.contains("--post-update 9.8.7"));
    QVERIFY2(process.readAllStandardError().isEmpty(), process.readAllStandardError().constData());
    QVERIFY(!QFileInfo::exists(staging));
    QVERIFY(!QFileInfo::exists(backup));
}

void UpdaterTransactionTest::rejectsPathOutsidePackageRoot()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-escape-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString escaped = QDir(dir.path()).absoluteFilePath(QStringLiteral("../escaped-backup"));
    const QString transaction = writeFixture(dir.path(), escaped);
    QVERIFY(!transaction.isEmpty());

    QProcess process;
    process.start(QStringLiteral(UPDATER_EXE), { QStringLiteral("--dry-run"), transaction });
    QVERIFY(process.waitForFinished(10000));
    QVERIFY(process.exitCode() != 0);
    QVERIFY(process.readAllStandardError().contains("escapes the package root"));
    QVERIFY(!QFileInfo::exists(escaped));
}

void UpdaterTransactionTest::stagesValidPackage()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-stage-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString transaction = writeFixture(
        dir.path(), QDir(dir.path()).filePath(QStringLiteral(".update/backup")));
    QVERIFY(!transaction.isEmpty());
    const QString package = QDir(dir.path()).filePath(
        QStringLiteral(".update/downloads/GameHQ-9.8.7-win64-update.zip"));
    const QByteArray manifest = R"({"schemaVersion":1,"productId":"underfusion.gamehq","appVersion":"9.8.7","layoutVersion":1,"minimumUpdaterVersion":"0.6.10"})";
    QVERIFY(writeZip(package, {{"GameHQ.exe", "launcher"}, {"app/GameHQ.exe", "application"},
                               {"update-package.json", manifest}, {"README.txt", "readme"}}));
    QVERIFY(syncTransactionHash(transaction, package));

    QScopedPointer<QProcess> process(runHelper(QStringLiteral("--stage"), transaction, this));
    QVERIFY(process->waitForFinished(10000));
    QCOMPARE(process->exitCode(), 0);
    QVERIFY(process->readAllStandardOutput().contains("STAGED AND VALIDATED"));
    QVERIFY(QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral(".update/staging/app/GameHQ.exe"))));
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe"))));
}

void UpdaterTransactionTest::rejectsTraversalAndRemovesStaging()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-traversal-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString transaction = writeFixture(
        dir.path(), QDir(dir.path()).filePath(QStringLiteral(".update/backup")));
    const QString package = QDir(dir.path()).filePath(
        QStringLiteral(".update/downloads/GameHQ-9.8.7-win64-update.zip"));
    QVERIFY(writeZip(package, {{"../escaped.txt", "hostile"}}));
    QVERIFY(syncTransactionHash(transaction, package));

    QScopedPointer<QProcess> process(runHelper(QStringLiteral("--stage"), transaction, this));
    QVERIFY(process->waitForFinished(10000));
    QVERIFY(process->exitCode() != 0);
    QVERIFY(process->readAllStandardError().contains("traversal"));
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral("escaped.txt"))));
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral(".update/staging"))));
}

void UpdaterTransactionTest::rejectsForbiddenDataAndBadManifest()
{
    const QList<QPair<QByteArray, QByteArray>> badEntries[] = {
        {{"GameHQ.exe", "launcher"}, {"app/GameHQ.exe", "application"},
         {"portable.flag", "1"}, {"update-package.json", "{}"}},
        {{"GameHQ.exe", "launcher"}, {"app/GameHQ.exe", "application"},
         {"update-package.json", R"({"schemaVersion":1,"productId":"underfusion.gamehq","appVersion":"9.8.6","layoutVersion":1,"minimumUpdaterVersion":"0.6.10"})"}}
    };
    for (const auto &entries : badEntries) {
        QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-invalid-XXXXXX")));
        QVERIFY(dir.isValid());
        const QString transaction = writeFixture(
            dir.path(), QDir(dir.path()).filePath(QStringLiteral(".update/backup")));
        const QString package = QDir(dir.path()).filePath(
            QStringLiteral(".update/downloads/GameHQ-9.8.7-win64-update.zip"));
        QVERIFY(writeZip(package, entries));
        QVERIFY(syncTransactionHash(transaction, package));
        QScopedPointer<QProcess> process(runHelper(QStringLiteral("--stage"), transaction, this));
        QVERIFY(process->waitForFinished(10000));
        QVERIFY(process->exitCode() != 0);
        QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral(".update/staging"))));
        QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe"))));
    }
}

void UpdaterTransactionTest::rejectsPackageChangedAfterVerification()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-hash-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString transaction = writeFixture(
        dir.path(), QDir(dir.path()).filePath(QStringLiteral(".update/backup")));
    const QString package = QDir(dir.path()).filePath(
        QStringLiteral(".update/downloads/GameHQ-9.8.7-win64-update.zip"));
    const QByteArray manifest = R"({"schemaVersion":1,"productId":"underfusion.gamehq","appVersion":"9.8.7","layoutVersion":1,"minimumUpdaterVersion":"0.6.10"})";
    QVERIFY(writeZip(package, {{"GameHQ.exe", "launcher"}, {"app/GameHQ.exe", "application"},
                               {"update-package.json", manifest}}));
    // Deliberately keep the transaction's hash of the earlier placeholder.
    QScopedPointer<QProcess> process(runHelper(QStringLiteral("--stage"), transaction, this));
    QVERIFY(process->waitForFinished(10000));
    QVERIFY(process->exitCode() != 0);
    QVERIFY(process->readAllStandardError().contains("SHA-256 changed"));
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral(".update/staging"))));
}

void UpdaterTransactionTest::snapshotsAndRestoresDataWithoutTouchingCaptures()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-data-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString data = QDir(dir.path()).filePath(QStringLiteral("gamehq-data"));
    const QString captures = QDir(dir.path()).filePath(QStringLiteral("Captures"));
    const QString snapshot = QDir(dir.path()).filePath(QStringLiteral(".update/data-snapshot"));
    QDir().mkpath(data);
    QDir().mkpath(captures);
    auto write = [](const QString &path, const QByteArray &contents) {
        QFile file(path); return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            && file.write(contents) == contents.size();
    };
    QVERIFY(write(QDir(data).filePath(QStringLiteral("config.json")), "old config"));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("gamehq.db")), "old database"));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("gamehq.db-wal")), "old wal"));
    const QString capture = QDir(captures).filePath(QStringLiteral("clip.mp4"));
    QVERIFY(write(capture, "user media"));
    std::string error;
    QVERIFY2(maintenance::begin(dir.path().toStdWString(), error), error.c_str());
    QVERIFY2(updater::createDataSnapshot(data.toStdWString(), snapshot.toStdWString(), error), error.c_str());
    QVERIFY(write(QDir(data).filePath(QStringLiteral("config.json")), "new config"));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("gamehq.db")), "migrated database"));
    QFile::remove(QDir(data).filePath(QStringLiteral("gamehq.db-wal")));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("gamehq.db-shm")), "new shm"));
    QVERIFY2(updater::restoreDataSnapshot(data.toStdWString(), snapshot.toStdWString(), error), error.c_str());
    auto read = [](const QString &path) { QFile file(path); file.open(QIODevice::ReadOnly); return file.readAll(); };
    QCOMPARE(read(QDir(data).filePath(QStringLiteral("config.json"))), QByteArray("old config"));
    QCOMPARE(read(QDir(data).filePath(QStringLiteral("gamehq.db"))), QByteArray("old database"));
    QCOMPARE(read(QDir(data).filePath(QStringLiteral("gamehq.db-wal"))), QByteArray("old wal"));
    QVERIFY(!QFileInfo::exists(QDir(data).filePath(QStringLiteral("gamehq.db-shm"))));
    QCOMPARE(read(capture), QByteArray("user media"));
}

void UpdaterTransactionTest::swapsOnlyOwnedProgramFiles()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-swap-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString staging = QDir(dir.path()).filePath(QStringLiteral(".update/staging"));
    const QString backup = QDir(dir.path()).filePath(QStringLiteral(".update/backup"));
    QDir().mkpath(QDir(staging).filePath(QStringLiteral("app")));
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral("app")));
    auto write = [](const QString &path, const QByteArray &data) { QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(data) == data.size(); };
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe")), "old launcher"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe")), "old app"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("portable.flag")), "keep"));
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("GameHQ.exe")), "new launcher"));
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("app/GameHQ.exe")), "new app"));
    updater::Transaction tx; tx.packageRoot = dir.path().toStdWString();
    tx.stagingDir = staging.toStdWString(); tx.backupDir = backup.toStdWString();
    tx.healthTokenPath = QDir(dir.path()).filePath(QStringLiteral(".update/healthy.token")).toStdWString();
    std::string error;
    QVERIFY2(updater::swapProgramFiles(tx, error), error.c_str());
    auto read = [](const QString &path) { QFile f(path); f.open(QIODevice::ReadOnly); return f.readAll(); };
    QCOMPARE(read(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe"))), QByteArray("new launcher"));
    QCOMPARE(read(QDir(backup).filePath(QStringLiteral("app/GameHQ.exe"))), QByteArray("old app"));
    QCOMPARE(read(QDir(dir.path()).filePath(QStringLiteral("portable.flag"))), QByteArray("keep"));
}

void UpdaterTransactionTest::lockedFileAbortsAndRollsBack()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-locked-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString staging = QDir(dir.path()).filePath(QStringLiteral(".update/staging"));
    const QString backup = QDir(dir.path()).filePath(QStringLiteral(".update/backup"));
    QDir().mkpath(staging);
    auto write = [](const QString &path, const QByteArray &data) { QFile f(path); return f.open(QIODevice::WriteOnly) && f.write(data) == data.size(); };
    const QString live = QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe"));
    const QString incoming = QDir(staging).filePath(QStringLiteral("GameHQ.exe"));
    QVERIFY(write(live, "old")); QVERIFY(write(incoming, "new"));
    HANDLE lock = CreateFileW(live.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    QVERIFY(lock != INVALID_HANDLE_VALUE);
    updater::Transaction tx; tx.packageRoot = dir.path().toStdWString();
    tx.stagingDir = staging.toStdWString(); tx.backupDir = backup.toStdWString();
    tx.healthTokenPath = QDir(dir.path()).filePath(QStringLiteral(".update/healthy.token")).toStdWString();
    std::string error;
    QVERIFY(!updater::swapProgramFiles(tx, error));
    CloseHandle(lock);
    QFile oldFile(live); QVERIFY(oldFile.open(QIODevice::ReadOnly)); QCOMPARE(oldFile.readAll(), QByteArray("old"));
    QFile newFile(incoming); QVERIFY(newFile.open(QIODevice::ReadOnly)); QCOMPARE(newFile.readAll(), QByteArray("new"));
    QVERIFY(!QFileInfo::exists(backup));
}

void UpdaterTransactionTest::healthyStartPublishesToken()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-health-XXXXXX")));
    QVERIFY(dir.isValid());
    updater::Transaction tx;
    tx.packageRoot = dir.path().toStdWString();
    tx.restartExecutable = QStringLiteral(HEALTH_FIXTURE_EXE).toStdWString();
    tx.healthTokenPath = QDir(dir.path()).filePath(QStringLiteral("healthy.token")).toStdWString();
    tx.expectedVersion = "9.8.7";
    std::string error;
    QVERIFY2(updater::launchAndWaitForHealth(tx, 5000, error), error.c_str());
    QFile token(QString::fromStdWString(tx.healthTokenPath.wstring()));
    QVERIFY(token.open(QIODevice::ReadOnly));
    QCOMPARE(token.readAll().trimmed(), QByteArray("9.8.7"));
}

void UpdaterTransactionTest::missingHealthTokenTimesOut()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-timeout-XXXXXX")));
    QVERIFY(dir.isValid());
    updater::Transaction tx;
    tx.packageRoot = dir.path().toStdWString();
    tx.restartExecutable = QStringLiteral(HEALTH_FIXTURE_EXE).toStdWString();
    tx.healthTokenPath = QDir(dir.path()).filePath(QStringLiteral("healthy.token")).toStdWString();
    tx.expectedVersion = "9.9.8";
    std::string error;
    QVERIFY(!updater::launchAndWaitForHealth(tx, 500, error));
    QVERIFY(QString::fromStdString(error).contains(QStringLiteral("timed out")));
}

void UpdaterTransactionTest::failedHealthRollbackRestoresProgramAndData()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-rollback-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString update = QDir(dir.path()).filePath(QStringLiteral(".update"));
    const QString staging = QDir(update).filePath(QStringLiteral("staging"));
    const QString backup = QDir(update).filePath(QStringLiteral("backup"));
    const QString snapshot = QDir(update).filePath(QStringLiteral("data-snapshot"));
    const QString data = QDir(dir.path()).filePath(QStringLiteral("gamehq-data"));
    const QString captures = QDir(dir.path()).filePath(QStringLiteral("Captures"));
    QDir().mkpath(QDir(staging).filePath(QStringLiteral("app")));
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral("app")));
    QDir().mkpath(data);
    QDir().mkpath(captures);
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile file(path); return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            && file.write(bytes) == bytes.size();
    };
    const QString liveExe = QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe"));
    QVERIFY(write(liveExe, "old program"));
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("GameHQ.exe")), "new program"));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("config.json")), "old config"));
    QVERIFY(write(QDir(captures).filePath(QStringLiteral("clip.mp4")), "user media"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("portable.flag")), "keep portable"));
    std::string error;
    QVERIFY2(updater::createDataSnapshot(data.toStdWString(), snapshot.toStdWString(), error), error.c_str());
    QVERIFY(write(QDir(data).filePath(QStringLiteral("config.json")), "new config"));
    updater::Transaction tx;
    tx.packageRoot = dir.path().toStdWString(); tx.stagingDir = staging.toStdWString();
    tx.backupDir = backup.toStdWString(); tx.dataDir = data.toStdWString();
    tx.dataSnapshotDir = snapshot.toStdWString();
    tx.healthTokenPath = QDir(update).filePath(QStringLiteral("healthy.token")).toStdWString();
    tx.restartExecutable = QStringLiteral(HEALTH_FIXTURE_EXE).toStdWString();
    QVERIFY2(updater::swapProgramFiles(tx, error), error.c_str());
    QVERIFY2(updater::writeTransactionPhase(tx, "validating", error), error.c_str());
    QVERIFY2(updater::recoverInterruptedUpdate(tx, error), error.c_str());
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(
        QDir(dir.path()).filePath(QStringLiteral("previous-started.token"))), 2000);
    QFile program(liveExe); QVERIFY(program.open(QIODevice::ReadOnly));
    QCOMPARE(program.readAll(), QByteArray("old program"));
    QFile config(QDir(data).filePath(QStringLiteral("config.json")));
    QVERIFY(config.open(QIODevice::ReadOnly)); QCOMPARE(config.readAll(), QByteArray("old config"));
    QFile clip(QDir(captures).filePath(QStringLiteral("clip.mp4")));
    QVERIFY(clip.open(QIODevice::ReadOnly)); QCOMPARE(clip.readAll(), QByteArray("user media"));
    QFile portable(QDir(dir.path()).filePath(QStringLiteral("portable.flag")));
    QVERIFY(portable.open(QIODevice::ReadOnly)); QCOMPARE(portable.readAll(), QByteArray("keep portable"));
    QVERIFY(!QFileInfo::exists(backup));
    QFile phase(QDir(update).filePath(QStringLiteral("transaction.phase")));
    QVERIFY(phase.open(QIODevice::ReadOnly)); QCOMPARE(phase.readAll().trimmed(), QByteArray("rolled_back"));
    QVERIFY(!QFileInfo::exists(QDir(update).filePath(QStringLiteral("maintenance.lock"))));
}

void UpdaterTransactionTest::interruptedMixedSwapRecoversPreviousProgram()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-interrupted-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString update = QDir(dir.path()).filePath(QStringLiteral(".update"));
    const QString staging = QDir(update).filePath(QStringLiteral("staging"));
    const QString backup = QDir(update).filePath(QStringLiteral("backup"));
    QDir().mkpath(QDir(staging).filePath(QStringLiteral("app")));
    QDir().mkpath(QDir(dir.path()).filePath(QStringLiteral("app")));
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile file(path); return file.open(QIODevice::WriteOnly)
            && file.write(bytes) == bytes.size();
    };
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe")), "old root"));
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe")), "old app"));
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("GameHQ.exe")), "new root"));
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("app/GameHQ.exe")), "new app"));
    updater::Transaction tx;
    tx.packageRoot = dir.path().toStdWString(); tx.stagingDir = staging.toStdWString();
    tx.backupDir = backup.toStdWString();
    tx.dataDir = QDir(dir.path()).filePath(QStringLiteral("gamehq-data")).toStdWString();
    tx.dataSnapshotDir = QDir(update).filePath(QStringLiteral("data-snapshot")).toStdWString();
    tx.healthTokenPath = QDir(update).filePath(QStringLiteral("healthy.token")).toStdWString();
    tx.restartExecutable = QStringLiteral(HEALTH_FIXTURE_EXE).toStdWString();
    std::string error;
    QVERIFY2(updater::swapProgramFiles(tx, error), error.c_str());
    QVERIFY(QDir().rename(QDir(dir.path()).filePath(QStringLiteral("app")),
                          QDir(staging).filePath(QStringLiteral("app"))));
    QVERIFY(QDir().rename(QDir(backup).filePath(QStringLiteral("app")),
                          QDir(dir.path()).filePath(QStringLiteral("app"))));
    QVERIFY2(updater::recoverInterruptedUpdate(tx, error), error.c_str());
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(
        QDir(dir.path()).filePath(QStringLiteral("previous-started.token"))), 2000);
    auto read = [](const QString &path) { QFile file(path); file.open(QIODevice::ReadOnly); return file.readAll(); };
    QCOMPARE(read(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe"))), QByteArray("old root"));
    QCOMPARE(read(QDir(dir.path()).filePath(QStringLiteral("app/GameHQ.exe"))), QByteArray("old app"));
    QVERIFY(!QFileInfo::exists(backup));
}

void UpdaterTransactionTest::rejectsPackageRequiringNewerUpdater()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-minimum-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString transaction = writeFixture(
        dir.path(), QDir(dir.path()).filePath(QStringLiteral(".update/backup")));
    const QString package = QDir(dir.path()).filePath(
        QStringLiteral(".update/downloads/GameHQ-9.8.7-win64-update.zip"));
    const QByteArray manifest = R"({"schemaVersion":1,"productId":"underfusion.gamehq","appVersion":"9.8.7","layoutVersion":1,"minimumUpdaterVersion":"99.0.0"})";
    QVERIFY(writeZip(package, {{"GameHQ.exe", "launcher"}, {"app/GameHQ.exe", "application"},
                               {"update-package.json", manifest}}));
    QVERIFY(syncTransactionHash(transaction, package));
    QScopedPointer<QProcess> process(runHelper(QStringLiteral("--stage"), transaction, this));
    QVERIFY(process->waitForFinished(10000));
    QVERIFY(process->exitCode() != 0);
    QVERIFY(process->readAllStandardError().contains("requires a newer updater helper"));
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral(".update/staging"))));
}

void UpdaterTransactionTest::promotesOnlySelfTestingPendingHelper()
{
    QTemporaryDir good(QDir::current().filePath(QStringLiteral("tst-updater-promote-XXXXXX")));
    QVERIFY(good.isValid());
    const QString current = QDir(good.path()).filePath(QStringLiteral("GameHQUpdater.exe"));
    const QString pending = QDir(good.path()).filePath(QStringLiteral("GameHQUpdater.pending.exe"));
    QFile old(current); QVERIFY(old.open(QIODevice::WriteOnly)); QVERIFY(old.write("old") == 3); old.close();
    QVERIFY(QFile::copy(QStringLiteral(UPDATER_EXE), pending));
    QVERIFY(launcher::promotePendingUpdater(good.path().toStdWString()));
    QVERIFY(!QFileInfo::exists(pending));
    QVERIFY(QFileInfo(current).size() > 3);

    QTemporaryDir bad(QDir::current().filePath(QStringLiteral("tst-updater-reject-helper-XXXXXX")));
    QVERIFY(bad.isValid());
    const QString badCurrent = QDir(bad.path()).filePath(QStringLiteral("GameHQUpdater.exe"));
    const QString badPending = QDir(bad.path()).filePath(QStringLiteral("GameHQUpdater.pending.exe"));
    QFile existing(badCurrent); QVERIFY(existing.open(QIODevice::WriteOnly)); QVERIFY(existing.write("keep") == 4); existing.close();
    // A real executable that deliberately rejects --self-test avoids involving
    // Windows' slow invalid-image compatibility path while testing the same gate.
    QVERIFY(QFile::copy(QStringLiteral(HEALTH_FIXTURE_EXE), badPending));
    QVERIFY(!launcher::promotePendingUpdater(bad.path().toStdWString()));
    QVERIFY(!QFileInfo::exists(badPending));
    QFile preserved(badCurrent); QVERIFY(preserved.open(QIODevice::ReadOnly));
    QCOMPARE(preserved.readAll(), QByteArray("keep"));
}

void UpdaterTransactionTest::completeApplyCleansStaleStagingAndPreservesUserData()
{
    QTemporaryDir dir(QDir::current().filePath(QStringLiteral("tst-updater-apply-XXXXXX")));
    QVERIFY(dir.isValid());
    const QString update = QDir(dir.path()).filePath(QStringLiteral(".update"));
    const QString backup = QDir(update).filePath(QStringLiteral("backup"));
    const QString transactionPath = writeFixture(dir.path(), backup);
    QVERIFY(!transactionPath.isEmpty());
    const QString data = QDir(dir.path()).filePath(QStringLiteral("gamehq-data"));
    const QString captures = QDir(dir.path()).filePath(QStringLiteral("Captures"));
    QDir().mkpath(captures);
    auto write = [](const QString &path, const QByteArray &bytes) {
        QFile file(path); return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            && file.write(bytes) == bytes.size();
    };
    QVERIFY(write(QDir(dir.path()).filePath(QStringLiteral("GameHQ.exe")), "old root"));
    QVERIFY(write(QDir(data).filePath(QStringLiteral("config.json")), "settings"));
    QVERIFY(write(QDir(captures).filePath(QStringLiteral("clip.mp4")), "user media"));
    const QString staging = QDir(update).filePath(QStringLiteral("staging"));
    QDir().mkpath(staging);
    QVERIFY(write(QDir(staging).filePath(QStringLiteral("abandoned.tmp")), "stale"));
    QFile fixture(QStringLiteral(HEALTH_FIXTURE_EXE));
    QVERIFY(fixture.open(QIODevice::ReadOnly));
    const QByteArray newLauncher = fixture.readAll();
    QVERIFY(!newLauncher.isEmpty());
    const QString package = QDir(update).filePath(
        QStringLiteral("downloads/GameHQ-9.8.7-win64-update.zip"));
    const QByteArray manifest = R"({"schemaVersion":1,"productId":"underfusion.gamehq","appVersion":"9.8.7","layoutVersion":1,"minimumUpdaterVersion":"0.6.10"})";
    QVERIFY(writeZip(package, {{"GameHQ.exe", newLauncher}, {"app/GameHQ.exe", "new app"},
                               {"update-package.json", manifest}}));
    QVERIFY(syncTransactionHash(transactionPath, package));
    updater::Transaction tx;
    std::string error;
    QVERIFY2(updater::loadAndValidateTransaction(transactionPath.toStdWString(), tx, error), error.c_str());
    QVERIFY2(maintenance::begin(dir.path().toStdWString(), error), error.c_str());
    QVERIFY2(updater::applyUpdate(tx, 5000, error), error.c_str());
    auto read = [](const QString &path) { QFile file(path); file.open(QIODevice::ReadOnly); return file.readAll(); };
    QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(QStringLiteral("portable.flag"))));
    QCOMPARE(read(QDir(data).filePath(QStringLiteral("config.json"))), QByteArray("settings"));
    QCOMPARE(read(QDir(captures).filePath(QStringLiteral("clip.mp4"))), QByteArray("user media"));
    QVERIFY(!QFileInfo::exists(QDir(staging).filePath(QStringLiteral("abandoned.tmp"))));
    QCOMPARE(read(QDir(update).filePath(QStringLiteral("transaction.phase"))).trimmed(), QByteArray("healthy"));
    QVERIFY(!QFileInfo::exists(QDir(update).filePath(QStringLiteral("maintenance.lock"))));
    QCOMPARE(read(QDir(backup).filePath(QStringLiteral("GameHQ.exe"))), QByteArray("old root"));
}

QTEST_GUILESS_MAIN(UpdaterTransactionTest)
#include "tst_updatertransaction.moc"
