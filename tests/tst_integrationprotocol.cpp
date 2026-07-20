#include <QtTest>

#include "integration/IntegrationProtocol.h"
#include "integration/LocalIntegrationServer.h"

#include <QJsonObject>
#include <QLocalSocket>
#include <QtEndian>

class IntegrationProtocolTest : public QObject
{
    Q_OBJECT
private slots:
    void fragmentedFrame();
    void multipleFrames();
    void invalidFrames_data();
    void invalidFrames();
    void boundedRandomInput();
    void liveServerAcceptsFrame();
    void liveServerDropsMalformedClient();
};

namespace
{
QByteArray prefix(quint32 length)
{
    QByteArray bytes(4, Qt::Uninitialized);
    qToLittleEndian<quint32>(length, bytes.data());
    return bytes;
}
}

void IntegrationProtocolTest::fragmentedFrame()
{
    QString error;
    const QByteArray frame = integration::encodeFrame(
        { { QStringLiteral("type"), QStringLiteral("hello") },
          { QStringLiteral("requestId"), QStringLiteral("one") } }, error);
    QVERIFY2(!frame.isEmpty(), qPrintable(error));
    integration::FrameDecoder decoder;
    QList<integration::Message> messages;
    for (char byte : frame)
        QVERIFY(decoder.append(QByteArray(1, byte), messages, error));
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().type, QStringLiteral("hello"));
    QCOMPARE(messages.first().requestId, QStringLiteral("one"));
}

void IntegrationProtocolTest::multipleFrames()
{
    QString error;
    const QByteArray first = integration::encodeFrame(
        { { QStringLiteral("type"), QStringLiteral("app.activate") } }, error);
    const QByteArray second = integration::encodeFrame(
        { { QStringLiteral("type"), QStringLiteral("status.request") } }, error);
    integration::FrameDecoder decoder;
    QList<integration::Message> messages;
    QVERIFY(decoder.append(first + second, messages, error));
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.at(1).type, QStringLiteral("status.request"));
}

void IntegrationProtocolTest::invalidFrames_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::newRow("zero") << prefix(0);
    QTest::newRow("oversized") << prefix(integration::MaximumPayloadBytes + 1);
    QTest::newRow("invalid-utf8") << (prefix(2) + QByteArray::fromHex("c328"));
    QTest::newRow("not-json") << (prefix(1) + QByteArray("x"));
    const QByteArray array("[]");
    QTest::newRow("not-object") << (prefix(array.size()) + array);
    const QByteArray missingType("{}");
    QTest::newRow("missing-type") << (prefix(missingType.size()) + missingType);
    const QByteArray unknown(R"({"type":"shell.execute"})");
    QTest::newRow("unknown-type") << (prefix(unknown.size()) + unknown);
    const QByteArray badRequest(R"({"type":"hello","requestId":7})");
    QTest::newRow("bad-request-id") << (prefix(badRequest.size()) + badRequest);
}

void IntegrationProtocolTest::invalidFrames()
{
    QFETCH(QByteArray, bytes);
    integration::FrameDecoder decoder;
    QList<integration::Message> messages;
    QString error;
    QVERIFY(!decoder.append(bytes, messages, error));
    QVERIFY(!error.isEmpty());
    QVERIFY(messages.isEmpty());
}

void IntegrationProtocolTest::boundedRandomInput()
{
    for (quint32 seed = 1; seed <= 500; ++seed) {
        quint32 value = seed * 2654435761U;
        QByteArray bytes;
        const int size = 4 + static_cast<int>(seed % 257);
        bytes.reserve(size);
        for (int i = 0; i < size; ++i) {
            value = value * 1664525U + 1013904223U;
            bytes.append(static_cast<char>(value >> 24));
        }
        integration::FrameDecoder decoder;
        QList<integration::Message> messages;
        QString error;
        decoder.append(bytes, messages, error);
        QVERIFY(messages.size() <= 1);
    }
}

void IntegrationProtocolTest::liveServerAcceptsFrame()
{
    LocalIntegrationServer server;
    QString error;
    QVERIFY2(server.start(error), qPrintable(error));
    bool received = false;
    connect(&server, &LocalIntegrationServer::messageReceived, this,
            [&received](quint64, const integration::Message &message) {
                received = message.type == QStringLiteral("hello");
            });
    QLocalSocket client;
    client.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName));
    QVERIFY(client.waitForConnected(1000));
    const QByteArray frame = integration::encodeFrame(
        { { QStringLiteral("type"), QStringLiteral("hello") } }, error);
    QCOMPARE(client.write(frame), frame.size());
    client.flush();
    QTRY_VERIFY_WITH_TIMEOUT(received, 1000);
}

void IntegrationProtocolTest::liveServerDropsMalformedClient()
{
    LocalIntegrationServer server;
    QString error;
    QVERIFY2(server.start(error), qPrintable(error));
    QLocalSocket client;
    client.connectToServer(QString::fromLatin1(LocalIntegrationServer::ServerName));
    QVERIFY(client.waitForConnected(1000));
    QCOMPARE(client.write(prefix(0)), 4);
    client.flush();
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QLocalSocket::UnconnectedState, 1000);
}

QTEST_GUILESS_MAIN(IntegrationProtocolTest)
#include "tst_integrationprotocol.moc"
