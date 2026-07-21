#include <QtTest>

#include "core/ApplicationMutex.h"

class ApplicationMutexTest : public QObject
{
    Q_OBJECT
private slots:
    void exposesProcessLifetimeSignal();
};

void ApplicationMutexTest::exposesProcessLifetimeSignal()
{
    if (ApplicationMutex::isHeld())
        QSKIP("A user GameHQ instance is running and owns the production installer mutex.");
    QVERIFY(!ApplicationMutex::isHeld());
    {
        ApplicationMutex first;
        QVERIFY(first.acquired());
        QVERIFY(ApplicationMutex::isHeld());

        ApplicationMutex second;
        QVERIFY(!second.acquired());
        QVERIFY(ApplicationMutex::isHeld());
    }
    QVERIFY(!ApplicationMutex::isHeld());
}

QTEST_APPLESS_MAIN(ApplicationMutexTest)
#include "tst_applicationmutex.moc"
