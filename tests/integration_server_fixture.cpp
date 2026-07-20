#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

#include "integration/IntegrationService.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    IntegrationService service(QStringLiteral("1.0.0"));
    QString error;
    if (!service.start(error)) {
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
