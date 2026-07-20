#include "integration/IntegrationClient.h"

#include "integration/IntegrationProtocol.h"
#include "integration/LocalIntegrationServer.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QStringDecoder>
#include <QtEndian>

namespace
{
constexpr int kConnectTimeoutMs = 250;
constexpr int kReplyTimeoutMs = 350;

int remaining(const QElapsedTimer &timer, int timeoutMs)
{
    return qMax(0, timeoutMs - static_cast<int>(timer.elapsed()));
}
}

bool IntegrationClient::writeObject(QLocalSocket &socket, const QJsonObject &object,
                                    int timeoutMs, QString &error)
{
    const QByteArray frame = integration::encodeFrame(object, error);
    if (frame.isEmpty())
        return false;
    if (socket.write(frame) != frame.size()) {
        error = QStringLiteral("could not queue the local integration message");
        return false;
    }
    socket.flush();
    if (socket.bytesToWrite() > 0 && !socket.waitForBytesWritten(timeoutMs)) {
        error = QStringLiteral("timed out sending the local integration message");
        return false;
    }
    return true;
}

bool IntegrationClient::readObject(QLocalSocket &socket, QJsonObject &object,
                                   int timeoutMs, QString &error)
{
    QElapsedTimer timer;
    timer.start();
    while (socket.bytesAvailable() < 4) {
        const int wait = remaining(timer, timeoutMs);
        if (wait <= 0 || !socket.waitForReadyRead(wait)) {
            error = QStringLiteral("timed out waiting for the running app");
            return false;
        }
    }
    const QByteArray prefix = socket.peek(4);
    const quint32 size = qFromLittleEndian<quint32>(prefix.constData());
    if (size == 0 || size > integration::MaximumPayloadBytes) {
        error = QStringLiteral("the running app returned an invalid frame length");
        return false;
    }
    while (socket.bytesAvailable() < 4 + size) {
        const int wait = remaining(timer, timeoutMs);
        if (wait <= 0 || !socket.waitForReadyRead(wait)) {
            error = QStringLiteral("the running app returned an incomplete frame");
            return false;
        }
    }
    socket.read(4);
    const QByteArray payload = socket.read(size);
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder.decode(payload);
    if (decoder.hasError()) {
        error = QStringLiteral("the running app returned invalid UTF-8");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("the running app returned invalid JSON");
        return false;
    }
    object = document.object();
    if (!object.value(QStringLiteral("type")).isString()) {
        error = QStringLiteral("the running app returned an invalid message type");
        return false;
    }
    return true;
}

bool IntegrationClient::forwardSecondInstance(const QStringList &arguments,
                                              const QString &appVersion,
                                              QString &error)
{
    error.clear();
    QLocalSocket socket;
    socket.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName),
                           QIODevice::ReadWrite);
    if (!socket.waitForConnected(kConnectTimeoutMs)) {
        error = QStringLiteral("the running app's local channel is unavailable");
        return false;
    }

    const QString requestPrefix = QStringLiteral("second-instance-%1-")
        .arg(QCoreApplication::applicationPid());
    const QJsonObject hello {
        { QStringLiteral("type"), QStringLiteral("hello") },
        { QStringLiteral("client"), QStringLiteral("GameHQ.SecondInstance") },
        { QStringLiteral("clientVersion"), appVersion },
        { QStringLiteral("protocolMin"), 1 },
        { QStringLiteral("protocolMax"), 1 },
        { QStringLiteral("requestId"), requestPrefix + QStringLiteral("hello") }
    };
    if (!writeObject(socket, hello, kReplyTimeoutMs, error))
        return false;
    QJsonObject reply;
    if (!readObject(socket, reply, kReplyTimeoutMs, error))
        return false;
    if (reply.value(QStringLiteral("type")).toString() != QStringLiteral("hello.ack")
        || reply.value(QStringLiteral("protocolSelected")).toInt() != 1) {
        error = reply.value(QStringLiteral("message")).toString(
            QStringLiteral("the running app rejected the integration handshake"));
        return false;
    }

    const bool openGallery = arguments.contains(QStringLiteral("--open-gallery"));
    QJsonObject action {
        { QStringLiteral("type"), openGallery ? QStringLiteral("app.open_gallery")
                                               : QStringLiteral("app.activate") },
        { QStringLiteral("requestId"), requestPrefix + QStringLiteral("action") }
    };
    if (arguments.contains(QStringLiteral("--post-update")))
        action.insert(QStringLiteral("intent"), QStringLiteral("post-update"));
    else if (arguments.contains(QStringLiteral("--show")))
        action.insert(QStringLiteral("intent"), QStringLiteral("show"));

    if (!writeObject(socket, action, kReplyTimeoutMs, error)
        || !readObject(socket, reply, kReplyTimeoutMs, error)) {
        return false;
    }
    if (reply.value(QStringLiteral("type")).toString() != QStringLiteral("ack")
        || reply.value(QStringLiteral("acceptedType")).toString()
            != action.value(QStringLiteral("type")).toString()) {
        error = reply.value(QStringLiteral("message")).toString(
            QStringLiteral("the running app did not acknowledge activation"));
        return false;
    }
    return true;
}
