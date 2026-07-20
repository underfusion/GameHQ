// GameHQ entry point — keep thin: hand off to App.
// QApplication (not QGuiApplication) because QSystemTrayIcon needs Widgets.
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLockFile>
#include <QSaveFile>
#include <QProcess>
#include <QTimer>
#include "app/App.h"
#include "Brand.h"
#include "config/Paths.h"
#include "input/HidCloakMonitor.h"
#include "integration/IntegrationClient.h"
#include "core/UpdateMaintenance.h"

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
