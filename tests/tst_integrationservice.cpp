#include <QtTest>

#include "integration/IntegrationProtocol.h"
#include "integration/IntegrationService.h"

#include <QJsonDocument>
#include <QLocalSocket>
#include <QtEndian>

class IntegrationServiceTest : public QObject
{
    Q_OBJECT
private slots:
    void handshakeActionsAndStatus();
    void incompatibleHandshakeCloses();
    void lifecycleSyncAndDisconnectExpiry();
    void externalIdentityRequiresProcessOrSafeGamePath();
};

namespace
{
void sendObject(QLocalSocket &socket, const QJsonObject &object)
{
    QString error;
    const QByteArray frame = integration::encodeFrame(object, error);
    QVERIFY2(!frame.isEmpty(), qPrintable(error));
    QCOMPARE(socket.write(frame), frame.size());
    socket.flush();
}

bool takeObject(QLocalSocket &socket, QJsonObject &object)
{
    if (socket.bytesAvailable() < 4 && !socket.waitForReadyRead(10))
        return false;
    const QByteArray prefix = socket.peek(4);
    const quint32 size = qFromLittleEndian<quint32>(prefix.constData());
    if (size == 0 || size > integration::MaximumPayloadBytes)
        return false;
    if (socket.bytesAvailable() < 4 + size)
        socket.waitForReadyRead(10);
    if (socket.bytesAvailable() < 4 + size)
        return false;
    socket.read(4);
    QJsonParseError error;
    const QByteArray payload = socket.read(size);
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    object = document.object();
    return true;
}

QJsonObject hello(int minimum = 1, int maximum = 1,
                  const QString &client = QStringLiteral("GameHQ.Test"))
{
    return {
        { QStringLiteral("type"), QStringLiteral("hello") },
        { QStringLiteral("client"), client },
        { QStringLiteral("clientVersion"), QStringLiteral("1.2.3") },
        { QStringLiteral("protocolMin"), minimum },
        { QStringLiteral("protocolMax"), maximum },
        { QStringLiteral("requestId"), QStringLiteral("hello-1") }
    };
}
}

void IntegrationServiceTest::handshakeActionsAndStatus()
{
    IntegrationService service(QStringLiteral("9.8.7"));
    QString error;
    QVERIFY2(service.start(error), qPrintable(error));
    QLocalSocket client;
    client.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName));
    QVERIFY(client.waitForConnected(1000));

    sendObject(client, { { QStringLiteral("type"), QStringLiteral("status.request") },
                         { QStringLiteral("requestId"), QStringLiteral("before") } });
    QJsonObject reply;
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));
    QCOMPARE(reply.value(QStringLiteral("code")).toString(), QStringLiteral("not_handshaken"));

    sendObject(client, hello());
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));
    QCOMPARE(reply.value(QStringLiteral("type")).toString(), QStringLiteral("hello.ack"));
    QCOMPARE(reply.value(QStringLiteral("appVersion")).toString(), QStringLiteral("9.8.7"));
    QCOMPARE(reply.value(QStringLiteral("protocolSelected")).toInt(), 1);
    QCOMPARE(reply.value(QStringLiteral("requestId")).toString(), QStringLiteral("hello-1"));

    bool activated = false;
    connect(&service, &IntegrationService::activateRequested,
            this, [&activated] { activated = true; });
    sendObject(client, { { QStringLiteral("type"), QStringLiteral("app.activate") },
                         { QStringLiteral("requestId"), QStringLiteral("activate-1") } });
    QTRY_VERIFY_WITH_TIMEOUT(activated, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));
    QCOMPARE(reply.value(QStringLiteral("type")).toString(), QStringLiteral("ack"));
    QCOMPARE(reply.value(QStringLiteral("requestId")).toString(), QStringLiteral("activate-1"));

    sendObject(client, { { QStringLiteral("type"), QStringLiteral("status.request") } });
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));
    QCOMPARE(reply.value(QStringLiteral("type")).toString(), QStringLiteral("status.response"));
    QCOMPARE(reply.value(QStringLiteral("externalGameCount")).toInt(), 0);
}

void IntegrationServiceTest::incompatibleHandshakeCloses()
{
    IntegrationService service(QStringLiteral("9.8.7"));
    QString error;
    QVERIFY2(service.start(error), qPrintable(error));
    QLocalSocket client;
    client.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName));
    QVERIFY(client.waitForConnected(1000));
    sendObject(client, hello(2, 3));
    QJsonObject reply;
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));
    QCOMPARE(reply.value(QStringLiteral("code")).toString(),
             QStringLiteral("protocol_incompatible"));
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QLocalSocket::UnconnectedState, 1000);
}

void IntegrationServiceTest::lifecycleSyncAndDisconnectExpiry()
{
    IntegrationService service(QStringLiteral("9.8.7"), nullptr, 50);
    QString error;
    QVERIFY2(service.start(error), qPrintable(error));
    QLocalSocket client;
    client.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName));
    QVERIFY(client.waitForConnected(1000));
    sendObject(client, hello(1, 1, QStringLiteral("GameHQ.Playnite")));
    QJsonObject reply;
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));

    sendObject(client, {
        { QStringLiteral("type"), QStringLiteral("playnite.game.started") },
        { QStringLiteral("sessionId"), QStringLiteral("session-a") },
        { QStringLiteral("playniteGameId"), QStringLiteral("game-a") },
        { QStringLiteral("name"), QStringLiteral("Example") },
        { QStringLiteral("startedProcessId"), 1234 }
    });
    QTRY_COMPARE_WITH_TIMEOUT(service.externalContext()->sessionCount(), 1, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));

    sendObject(client, {
        { QStringLiteral("type"), QStringLiteral("playnite.state.sync") },
        { QStringLiteral("games"), QJsonArray{} }
    });
    QTRY_COMPARE_WITH_TIMEOUT(service.externalContext()->sessionCount(), 0, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= 4, 1000);
    QVERIFY(takeObject(client, reply));

    sendObject(client, {
        { QStringLiteral("type"), QStringLiteral("playnite.game.started") },
        { QStringLiteral("sessionId"), QStringLiteral("session-b") },
        { QStringLiteral("name"), QStringLiteral("Example Two") }
    });
    QTRY_COMPARE_WITH_TIMEOUT(service.externalContext()->sessionCount(), 1, 1000);
    client.abort();
    QTRY_COMPARE_WITH_TIMEOUT(service.externalContext()->sessionCount(), 0, 1000);
}

void IntegrationServiceTest::externalIdentityRequiresProcessOrSafeGamePath()
{
    integration::ExternalGameContext context;
    QString error;
    QVERIFY(context.upsert(QStringLiteral("GameHQ.Playnite"), {
        { QStringLiteral("sessionId"), QStringLiteral("session-a") },
        { QStringLiteral("playniteGameId"), QStringLiteral("game-a") },
        { QStringLiteral("name"), QStringLiteral("Trusted Title") },
        { QStringLiteral("installDirectory"), QStringLiteral("C:/Games/Trusted") },
        { QStringLiteral("startedProcessId"), 55 }
    }, QStringLiteral("started"), error));

    auto noDescendant = [](quint32, quint32) { return false; };
    const auto exact = context.matchForeground(
        55, QStringLiteral("C:/Unrelated/browser.exe"), false, noDescendant);
    QCOMPARE(exact.confidence, integration::MatchConfidence::ExactProcess);
    QVERIFY(exact.authorizesWindowedCapture());

    const auto descendant = context.matchForeground(
        77, QStringLiteral("C:/Unrelated/child.exe"), false,
        [](quint32 child, quint32 ancestor) { return child == 77 && ancestor == 55; });
    QCOMPARE(descendant.confidence, integration::MatchConfidence::DescendantProcess);
    QVERIFY(descendant.authorizesWindowedCapture());

    const auto nameOnly = context.matchForeground(
        99, QStringLiteral("C:/Unrelated/browser.exe"), false, noDescendant);
    QCOMPARE(nameOnly.confidence, integration::MatchConfidence::None);

    const auto pathWithoutGameGate = context.matchForeground(
        99, QStringLiteral("C:/Games/Trusted/game.exe"), false, noDescendant);
    QCOMPARE(pathWithoutGameGate.confidence, integration::MatchConfidence::None);
    const auto safePath = context.matchForeground(
        99, QStringLiteral("C:/Games/Trusted/game.exe"), true, noDescendant);
    QCOMPARE(safePath.confidence, integration::MatchConfidence::InstallDirectory);
    QVERIFY(!safePath.authorizesWindowedCapture());

    // relativeFilePath() returns an absolute path for another drive; that must
    // never count as "inside" the install directory.
    const auto crossDrive = context.matchForeground(
        99, QStringLiteral("D:/Other/game.exe"), true, noDescendant);
    QCOMPARE(crossDrive.confidence, integration::MatchConfidence::None);
}

QTEST_GUILESS_MAIN(IntegrationServiceTest)
#include "tst_integrationservice.moc"
