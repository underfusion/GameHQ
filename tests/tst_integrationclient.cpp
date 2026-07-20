#include <QtTest>

#include "integration/IntegrationClient.h"

#include <QElapsedTimer>
#include <QProcess>

class IntegrationClientTest : public QObject
{
    Q_OBJECT
private slots:
    void forwardsActivation_data();
    void forwardsActivation();
    void unavailableServerIsBounded();
};

void IntegrationClientTest::forwardsActivation_data()
{
    QTest::addColumn<QString>("argument");
    QTest::newRow("show") << QStringLiteral("--show");
    QTest::newRow("gallery") << QStringLiteral("--open-gallery");
}

void IntegrationClientTest::forwardsActivation()
{
    QFETCH(QString, argument);
    QProcess fixture;
    fixture.setProgram(QStringLiteral(INTEGRATION_SERVER_FIXTURE));
    fixture.start();
    QVERIFY(fixture.waitForStarted(1000));
    QVERIFY(fixture.waitForReadyRead(1000));
    QCOMPARE(fixture.readLine().trimmed(), QByteArray("READY"));

    QString error;
    QVERIFY2(IntegrationClient::forwardSecondInstance(
                 { QStringLiteral("GameHQ.exe"), argument },
                 QStringLiteral("1.2.3"), error), qPrintable(error));
    QVERIFY(fixture.waitForFinished(1000));
    QCOMPARE(fixture.exitCode(), 0);
}

void IntegrationClientTest::unavailableServerIsBounded()
{
    QElapsedTimer timer;
    timer.start();
    QString error;
    QVERIFY(!IntegrationClient::forwardSecondInstance(
        { QStringLiteral("GameHQ.exe") }, QStringLiteral("1.2.3"), error));
    QVERIFY(!error.isEmpty());
    QVERIFY(timer.elapsed() < 1000);
}

QTEST_GUILESS_MAIN(IntegrationClientTest)
#include "tst_integrationclient.moc"
