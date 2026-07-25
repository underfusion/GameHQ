#include "core/ProcessIdentity.h"

#include <QElapsedTimer>
#include <QProcess>
#include <QString>
#include <QTest>

#include <windows.h>

// The portable import waits for the running GameHQ to close before it replaces
// the data folder. It used to do that with a bare process id: Windows reuses
// ids, so the wait could land on an unrelated program, and OpenProcess failing
// for any reason at all - including "you may not look at this process" - was
// read as "the parent is gone", which let the import start while the old
// instance still had the folder open.
class TestProcessIdentity : public QObject
{
    Q_OBJECT

private:
    using Outcome = ProcessIdentity::WaitOutcome;

    static QString creationTimeOfToken(const QString& token)
    {
        return token.section(QLatin1Char(':'), 1);
    }

private slots:
    void aTokenNamesThisProcessAndParsesBack()
    {
        const QString token = ProcessIdentity::currentToken();
        QVERIFY(!token.isEmpty());

        quint32 processId = 0;
        quint64 creationTime = 0;
        QVERIFY(ProcessIdentity::parseToken(token, processId, creationTime));
        QCOMPARE(processId, quint32(GetCurrentProcessId()));
        QVERIFY(creationTime != 0);
        QCOMPARE(ProcessIdentity::tokenFor(quint32(GetCurrentProcessId())), token);
    }

    void rubbishIsRejectedRatherThanGuessedAt()
    {
        quint32 processId = 0;
        quint64 creationTime = 0;
        for (const QString& token : { QString(), QStringLiteral("1234"),
                                      QStringLiteral("1234:"), QStringLiteral(":99"),
                                      QStringLiteral("0:99"), QStringLiteral("abc:99"),
                                      QStringLiteral("1234:zz"), QStringLiteral("1:2:3") }) {
            QVERIFY2(!ProcessIdentity::parseToken(token, processId, creationTime),
                     qPrintable(QStringLiteral("accepted: ") + token));
            QCOMPARE(ProcessIdentity::waitForExit(token, 0), Outcome::Malformed);
        }
    }

    void aLiveProcessIsNeverReportedAsExited()
    {
        // This very process: alive by definition, so the wait must time out
        // rather than claim success.
        QElapsedTimer elapsed;
        elapsed.start();
        QCOMPARE(ProcessIdentity::waitForExit(ProcessIdentity::currentToken(), 150),
                 Outcome::StillRunning);
        QVERIFY(elapsed.elapsed() >= 100);
    }

    void aRecycledIdIsTreatedAsGoneInsteadOfWaitedOn()
    {
        // Same id as this live process, but the creation time of a different
        // one. Waiting on it would block on a stranger for the full timeout;
        // the process actually named is long gone.
        const QString recycled = QStringLiteral("%1:%2")
                                     .arg(GetCurrentProcessId())
                                     .arg(quint64(123456789));
        QElapsedTimer elapsed;
        elapsed.start();
        QCOMPARE(ProcessIdentity::waitForExit(recycled, 5000), Outcome::Exited);
        QVERIFY(elapsed.elapsed() < 1000);
    }

    void aProcessThatHasExitedIsReportedExited()
    {
        QProcess child;
        child.start(QStringLiteral(PROCESS_IDENTITY_FIXTURE), { QStringLiteral("100") });
        QVERIFY2(child.waitForStarted(10000), qPrintable(child.errorString()));

        const QString token = ProcessIdentity::tokenFor(quint32(child.processId()));
        QVERIFY(!token.isEmpty());
        // While it runs, a short wait must say so rather than proceed.
        QCOMPARE(ProcessIdentity::waitForExit(token, 10), Outcome::StillRunning);

        // And the wait really does return as soon as it ends.
        QCOMPARE(ProcessIdentity::waitForExit(token, 30000), Outcome::Exited);
        QVERIFY(child.waitForFinished(10000));

        // Once the id is free, the same token still resolves to "gone" rather
        // than to whatever process inherits that id next.
        QCOMPARE(ProcessIdentity::waitForExit(token, 5000), Outcome::Exited);
    }

    void aProcessThatCannotBeInspectedIsNeverAssumedGone()
    {
        // The System process (id 4) exists and refuses inspection from an
        // ordinary user. The old code read that refusal as "already exited".
        HANDLE system = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 4);
        if (system) {
            CloseHandle(system);
            QSKIP("This session may inspect the System process, so the denial path cannot be exercised.");
        }
        QCOMPARE(ProcessIdentity::waitForExit(QStringLiteral("4:1"), 0), Outcome::Unverifiable);
    }
};

QTEST_GUILESS_MAIN(TestProcessIdentity)
#include "tst_processidentity.moc"
