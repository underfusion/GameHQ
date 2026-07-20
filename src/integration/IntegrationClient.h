#pragma once

#include <QString>
#include <QStringList>

class QLocalSocket;
class QJsonObject;

class IntegrationClient
{
public:
    static bool forwardSecondInstance(const QStringList &arguments,
                                      const QString &appVersion,
                                      QString &error);

private:
    static bool writeObject(QLocalSocket &socket, const QJsonObject &object,
                            int timeoutMs, QString &error);
    static bool readObject(QLocalSocket &socket, QJsonObject &object,
                           int timeoutMs, QString &error);
};
