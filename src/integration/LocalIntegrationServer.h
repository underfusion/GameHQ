#pragma once

#include "integration/IntegrationProtocol.h"

#include <QHash>
#include <QLocalServer>
#include <QObject>

class QLocalSocket;

class LocalIntegrationServer final : public QObject
{
    Q_OBJECT
public:
    static constexpr auto ServerName = "GameHQ.Local.v1";

    explicit LocalIntegrationServer(QObject *parent = nullptr);
    ~LocalIntegrationServer() override;

    bool start(QString &error);
    void stop();
    bool send(quint64 clientId, const QJsonObject &message);
    void disconnectClient(quint64 clientId);
    void broadcast(const QJsonObject &message);
    int clientCount() const { return m_clients.size(); }

signals:
    void clientConnected(quint64 clientId);
    void messageReceived(quint64 clientId, const integration::Message &message);
    void clientDisconnected(quint64 clientId);

private:
    struct ClientState
    {
        quint64 id = 0;
        integration::FrameDecoder decoder;
    };

    void acceptConnections();
    void readClient(QLocalSocket *socket);
    void rejectClient(QLocalSocket *socket, const QString &reason);

    QLocalServer m_server;
    QHash<QLocalSocket *, ClientState> m_clients;
    quint64 m_nextClientId = 1;
};
