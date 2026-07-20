#include "integration/LocalIntegrationServer.h"

#include <QJsonObject>
#include <QLocalSocket>

#include <utility>

namespace
{
constexpr int kMaximumClients = 8;
}

LocalIntegrationServer::LocalIntegrationServer(QObject *parent)
    : QObject(parent)
{
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    m_server.setMaxPendingConnections(kMaximumClients);
    connect(&m_server, &QLocalServer::newConnection,
            this, &LocalIntegrationServer::acceptConnections);
}

LocalIntegrationServer::~LocalIntegrationServer()
{
    stop();
}

bool LocalIntegrationServer::start(QString &error)
{
    error.clear();
    if (m_server.isListening())
        return true;
    if (!m_server.listen(QString::fromLatin1(ServerName))) {
        error = m_server.errorString();
        return false;
    }
    qInfo() << "Integration server listening on" << ServerName
            << "with same-user access";
    return true;
}

void LocalIntegrationServer::stop()
{
    m_server.close();
    const auto sockets = m_clients.keys();
    for (QLocalSocket *socket : sockets) {
        socket->disconnect(this);
        socket->abort();
        socket->deleteLater();
    }
    m_clients.clear();
}

void LocalIntegrationServer::acceptConnections()
{
    while (QLocalSocket *socket = m_server.nextPendingConnection()) {
        if (m_clients.size() >= kMaximumClients) {
            qWarning() << "Integration client rejected: connection limit reached";
            socket->abort();
            socket->deleteLater();
            continue;
        }
        ClientState state;
        state.id = m_nextClientId++;
        m_clients.insert(socket, state);
        emit clientConnected(state.id);
        connect(socket, &QLocalSocket::readyRead, this,
                [this, socket] { readClient(socket); });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket] {
            const auto it = m_clients.find(socket);
            if (it != m_clients.end()) {
                const quint64 id = it->id;
                m_clients.erase(it);
                emit clientDisconnected(id);
            }
            socket->deleteLater();
        });
    }
}

void LocalIntegrationServer::disconnectClient(quint64 clientId)
{
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (it->id == clientId) {
            it.key()->disconnectFromServer();
            return;
        }
    }
}

void LocalIntegrationServer::readClient(QLocalSocket *socket)
{
    auto it = m_clients.find(socket);
    if (it == m_clients.end())
        return;
    QList<integration::Message> messages;
    QString error;
    if (!it->decoder.append(socket->readAll(), messages, error)) {
        rejectClient(socket, error);
        return;
    }
    for (const integration::Message &message : std::as_const(messages))
        emit messageReceived(it->id, message);
}

void LocalIntegrationServer::rejectClient(QLocalSocket *socket, const QString &reason)
{
    qWarning() << "Integration client disconnected:" << reason;
    socket->abort();
}

bool LocalIntegrationServer::send(quint64 clientId, const QJsonObject &message)
{
    QLocalSocket *target = nullptr;
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (it->id == clientId) {
            target = it.key();
            break;
        }
    }
    if (!target || target->state() != QLocalSocket::ConnectedState)
        return false;
    QString error;
    const QByteArray frame = integration::encodeFrame(message, error);
    if (frame.isEmpty()) {
        qWarning() << "Integration reply rejected:" << error;
        return false;
    }
    if (target->bytesToWrite() > integration::MaximumBufferedBytes - frame.size()) {
        rejectClient(target, QStringLiteral("outbound queue exceeded its limit"));
        return false;
    }
    const qint64 written = target->write(frame);
    if (written != frame.size())
        return false;
    target->flush();
    return true;
}

void LocalIntegrationServer::broadcast(const QJsonObject &message)
{
    QList<quint64> ids;
    ids.reserve(m_clients.size());
    for (const ClientState &state : std::as_const(m_clients))
        ids.push_back(state.id);
    for (quint64 id : std::as_const(ids))
        send(id, message);
}
