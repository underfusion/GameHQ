// GameHQ entry point — keep thin: hand off to App.
// QApplication (not QGuiApplication) because QSystemTrayIcon needs Widgets.
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLockFile>
#include "app/App.h"
#include "Brand.h"
#include "input/HidCloakMonitor.h"

#include <cstring>

int main(int argc, char* argv[])
{
    // Elevated helper mode (no GUI, no single-instance lock): the Settings
    // "Fix automatically" button relaunches this exe with "runas" so the
    // HidHide whitelist IOCTL runs with admin rights (docs/controller-input.md).
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--hidhide-allow-self") == 0)
            return HidCloakMonitor::applyWhitelistSelfElevated();
    }

    QApplication qtApp(argc, argv);

    // Single-instance guard: a second launch would lose the fight for the
    // global hotkey and tray icon, so it just exits quietly.
    QLockFile instanceLock(QDir::tempPath() + QStringLiteral("/gamehq.lock"));
    if (!instanceLock.tryLock(100))
        return 0;
    QApplication::setApplicationName(QString::fromLatin1(Brand::Name));
    QApplication::setOrganizationName(QString::fromLatin1(Brand::Name));
    QApplication::setApplicationVersion(QStringLiteral(GAMEHQ_VERSION));
    QApplication::setQuitOnLastWindowClosed(false); // tray-resident by design
    // Default icon for every window → title bar + taskbar (tray sets its own too).
    // The .ico carries pre-rendered 16–256 px frames, so every surface gets a
    // crisp raster instead of relying on the SVG icon engine at odd sizes.
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/gamehq.ico")));

    App app;
    if (!app.init())
        return 1;

    return qtApp.exec();
}
