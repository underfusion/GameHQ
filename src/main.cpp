// GameHQ entry point — keep thin: hand off to App.
// QApplication (not QGuiApplication) because QSystemTrayIcon needs Widgets.
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLockFile>
#include <QMessageBox>
#include <QSaveFile>
#include <QProcess>
#include <QTimer>
#include "app/App.h"
#include "Brand.h"
#include "config/Paths.h"
#include "config/PortableProfileImporter.h"
#include "input/HidCloakMonitor.h"
#include "integration/IntegrationClient.h"
#include "core/ApplicationMutex.h"
#include "core/UpdateMaintenance.h"
#include "security/ReleaseTrust.h"

#include <cstring>
#include <iostream>
#include <windows.h>

namespace
{
bool isPostUpdateTokenPath(const QString& path)
{
    const QString updateRoot = QFileInfo(Paths::packageRoot()
        + QStringLiteral("/.update")).absoluteFilePath();
    const QString token = QFileInfo(path).absoluteFilePath();
    const QString relative = QDir(updateRoot).relativeFilePath(token);
    return relative != QStringLiteral("..")
        && !relative.startsWith(QStringLiteral("../"))
        && relative != QStringLiteral(".");
}

QString updatePhase()
{
    QFile file(Paths::packageRoot() + QStringLiteral("/.update/transaction.phase"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readLine()).trimmed();
}

bool interruptedUpdateExists()
{
    const QString phase = updatePhase();
    return (!phase.isEmpty() && phase != QStringLiteral("healthy")
            && phase != QStringLiteral("rolled_back"))
        || QFileInfo::exists(Paths::packageRoot() + QStringLiteral("/.update/swap.manifest"));
}

void cleanCompletedUpdateArtifacts()
{
    const QString update = Paths::packageRoot() + QStringLiteral("/.update");
    for (const QString &name : { QStringLiteral("backup"), QStringLiteral("data-snapshot"),
                                 QStringLiteral("staging"), QStringLiteral("failed-new"),
                                 QStringLiteral("downloads") })
        QDir(update + QLatin1Char('/') + name).removeRecursively();
    // updater.log is deliberately retained for diagnostics.
    for (const QString &name : { QStringLiteral("swap.manifest"),
                                 QStringLiteral("healthy.token"),
                                 QStringLiteral("transaction.phase"),
                                 QStringLiteral("transaction.json") })
        QFile::remove(update + QLatin1Char('/') + name);
}

bool waitForParentProcess(const QStringList& arguments, QString& error)
{
    const qsizetype at = arguments.indexOf(QStringLiteral("--wait-for-pid"));
    if (at < 0)
        return true;
    if (at + 1 >= arguments.size()) {
        error = QStringLiteral("The portable import parent-process argument is incomplete.");
        return false;
    }
    bool ok = false;
    const DWORD pid = arguments.at(at + 1).toULong(&ok);
    if (!ok || pid == 0) {
        error = QStringLiteral("The portable import parent-process identifier is invalid.");
        return false;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!process)
        return true;
    const DWORD waitResult = WaitForSingleObject(process, 60000);
    CloseHandle(process);
    if (waitResult != WAIT_OBJECT_0) {
        error = QStringLiteral("The running GameHQ instance did not close in time.");
        return false;
    }
    return true;
}
}

int main(int argc, char* argv[])
{
    // Parse this packaging probe from the raw Windows command line. GUI-
    // subsystem CRT wrappers are not consistent about populating narrow argv
    // before QApplication exists.
    const std::wstring rawCommand = GetCommandLineW();
    if (rawCommand.find(L"--assert-version") != std::wstring::npos) {
        #define GAMEHQ_WIDEN_IMPL(value) L##value
        #define GAMEHQ_WIDEN(value) GAMEHQ_WIDEN_IMPL(value)
        const std::wstring expected = GAMEHQ_WIDEN(GAMEHQ_VERSION);
        #undef GAMEHQ_WIDEN
        #undef GAMEHQ_WIDEN_IMPL
        return rawCommand.find(L"--assert-version " + expected) != std::wstring::npos ? 0 : 2;
    }
    if (rawCommand.find(L"--release-trust-self-test") != std::wstring::npos) {
        std::string error;
        if (!release_trust::runBuiltInSelfTest(error)) {
            std::cerr << "Release trust self-test failed: " << error << '\n';
            return 6;
        }
        return 0;
    }
    // Elevated helper mode (no GUI, no single-instance lock): the Settings
    // "Fix automatically" button relaunches this exe with "runas" so the
    // HidHide whitelist IOCTL runs with admin rights (docs/controller-input.md).
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--hidhide-allow-self") == 0)
            return HidCloakMonitor::applyWhitelistSelfElevated();
    }

    QApplication qtApp(argc, argv);
    QApplication::setApplicationName(QString::fromLatin1(Brand::Name));
    QApplication::setOrganizationName(QString::fromLatin1(Brand::Name));
    QApplication::setApplicationVersion(QStringLiteral(GAMEHQ_VERSION));
    QApplication::setQuitOnLastWindowClosed(false); // tray-resident by design
    // Default icon for every window → title bar + taskbar (tray sets its own too).
    // The .ico carries pre-rendered 16–256 px frames, so every surface gets a
    // crisp raster instead of relying on the SVG icon engine at odd sizes.
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/gamehq.ico")));

    bool postUpdateValidation = false;
    QString postUpdateVersion;
    QString postUpdateToken;
    const QStringList arguments = qtApp.arguments();
    const qsizetype importAt = arguments.indexOf(QStringLiteral("--import-portable"));
    if (importAt >= 0) {
        QString importError;
        if (importAt + 1 >= arguments.size())
            importError = QStringLiteral("The portable import source folder is missing.");
        else if (Paths::isPortable())
            importError = QStringLiteral("Run portable import from an installed copy of GameHQ.");
        else if (waitForParentProcess(arguments, importError)) {
            PortableProfileImporter::Result result;
            PortableProfileImporter::Options options {
                arguments.at(importAt + 1), Paths::dataDir(),
                PortableProfileImporter::FailurePoint::None
            };
            PortableProfileImporter::importProfile(options, result, importError);
        }
        if (!importError.isEmpty()) {
            if (arguments.contains(QStringLiteral("--import-portable-only"))) {
                qCritical().noquote() << importError;
                return 7;
            }
            QMessageBox::critical(nullptr, QStringLiteral("Portable import failed"), importError);
            return 7;
        }
        if (arguments.contains(QStringLiteral("--import-portable-only")))
            return 0;
    }
    const qsizetype postUpdateAt = arguments.indexOf(QStringLiteral("--post-update"));
    if (postUpdateAt >= 0) {
        if (postUpdateAt + 2 >= arguments.size())
            return 2;
        postUpdateVersion = arguments.at(postUpdateAt + 1);
        postUpdateToken = arguments.at(postUpdateAt + 2);
        if (postUpdateVersion != QStringLiteral(GAMEHQ_VERSION)
            || !isPostUpdateTokenPath(postUpdateToken))
            return 2;
        postUpdateValidation = true;
    }

    if (!postUpdateValidation && interruptedUpdateExists()) {
        const QString helper = Paths::packageRoot() + QStringLiteral("/GameHQUpdater.exe");
        const QString transaction = Paths::packageRoot() + QStringLiteral("/.update/transaction.json");
        if (!QFileInfo(helper).isExecutable() || !QFileInfo(transaction).isFile())
            return 3;
        // Recovery restarts the previous version itself. Run before taking the
        // app lock so that restarted process is not rejected as a duplicate.
        return QProcess::execute(helper, { QStringLiteral("--recover"), transaction }) == 0 ? 0 : 3;
    }
    const QString phaseAtStart = updatePhase();
    const bool completedUpdateAtStart = !postUpdateValidation
        && (phaseAtStart == QStringLiteral("healthy")
            || phaseAtStart == QStringLiteral("rolled_back"));

    if (!postUpdateValidation) {
        const maintenance::Info state = maintenance::inspect(
            Paths::packageRoot().toStdWString());
        if (state.state == maintenance::State::Active)
            return 4;
    }

    // QLockFile remains the source of truth. A rejected second process only
    // uses the local channel to ask the existing UI to come forward, then exits.
    QLockFile instanceLock(QDir::tempPath() + QStringLiteral("/gamehq.lock"));
    if (!instanceLock.tryLock(100)) {
        QString forwardingError;
        if (!IntegrationClient::forwardSecondInstance(arguments,
                                                       QStringLiteral(GAMEHQ_VERSION),
                                                       forwardingError)) {
            qWarning() << "Second-instance activation was not acknowledged:"
                       << forwardingError;
        }
        return 0;
    }

    // Setup and Uninstall observe this process-lifetime mutex. It is acquired
    // only after QLockFile accepts this copy, so rejected second instances do
    // not make the installer think GameHQ is still running.
    ApplicationMutex applicationMutex;
    if (!applicationMutex.acquired())
        return 5;

    App app;
    app.setPostUpdateValidation(postUpdateValidation);
    if (!app.init())
        return 1;

    if (postUpdateValidation) {
        QTimer::singleShot(7000, &app, [&app, postUpdateVersion, postUpdateToken] {
            // Persist the one-time greeting before publishing health: a token
            // means all post-update bookkeeping is durable too.
            app.recordPostUpdateSuccess(postUpdateVersion);
            QSaveFile token(postUpdateToken);
            const QByteArray contents = postUpdateVersion.toUtf8() + '\n';
            if (!token.open(QIODevice::WriteOnly)
                || token.write(contents) != contents.size()
                || !token.commit()) {
                qCritical() << "Post-update validation could not publish its health token";
                return;
            }
            qInfo() << "Post-update validation succeeded for" << postUpdateVersion;
            app.completePostUpdateValidation();
        });
    } else if (completedUpdateAtStart) {
        // Retain rollback material through the first validation run. Only a
        // subsequent start that also survives seven seconds may discard it.
        QTimer::singleShot(7000, &app, [] {
            cleanCompletedUpdateArtifacts();
            qInfo() << "Previous update recovery artifacts cleaned after a second healthy start";
        });
    }

    return qtApp.exec();
}
