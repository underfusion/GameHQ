#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

#include "integration/IntegrationService.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    // argv[1] is the pipe name to listen on. The test harness always passes a
    // unique per-run name so this fixture never collides with a real running
    // GameHQ.exe on the production "GameHQ.Local.v1" pipe.
    const QString serverName = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    IntegrationService service(QStringLiteral("1.0.0"));
    QString error;
    if (!service.start(error, serverName)) {
        QTextStream(stderr) << error << Qt::endl;
        return 3;
    }
    QObject::connect(&service, &IntegrationService::activateRequested,
                     &app, [&app] { app.exit(0); });
    QObject::connect(&service, &IntegrationService::openGalleryRequested,
                     &app, [&app] { app.exit(0); });
    QTimer::singleShot(3000, &app, [&app] { app.exit(2); });
    QTextStream(stdout) << "READY" << Qt::endl;
    return app.exec();
}
