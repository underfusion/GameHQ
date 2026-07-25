#include "capture/CapturePublisher.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>

#include <atomic>

// A screenshot used to be named with an exists()-then-write test and encoded
// straight to that final name. Two encoder threads finishing inside the same
// second could therefore be handed the same path, and any interrupted encode
// left a truncated image sitting where the scanner indexes captures.
class TestCapturePublisher : public QObject
{
    Q_OBJECT

private slots:
    void parallelReservationsNeverShareAName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // More contenders than the two real encoder threads, all inside one
        // timestamp tick, which is exactly the case the old check lost. Keep
        // this bounded: Windows CI runners may refuse a 32-thread burst before
        // the test reaches the code it is meant to exercise.
        constexpr int kThreads = 8;
        QVector<CapturePublisher::Reservation> results(kThreads);
        std::atomic_int ready{0};
        QThreadPool pool;
        pool.setMaxThreadCount(kThreads);
        for (int i = 0; i < kThreads; ++i) {
            pool.start([&, i]() {
                // Line the workers up so they reserve at the same moment, but
                // never wait forever if the pool hands out fewer threads.
                ++ready;
                QElapsedTimer waited;
                waited.start();
                while (ready.load() < kThreads && waited.elapsed() < 2000) { }
                results[i] = CapturePublisher::reserve(dir.path(),
                                                       QStringLiteral("yyyy-MM-dd_HH-mm-ss"),
                                                       QStringLiteral(".png"));
            });
        }
        QVERIFY(pool.waitForDone(30000));

        QSet<QString> finals;
        QSet<QString> pendings;
        for (const CapturePublisher::Reservation& reservation : results) {
            QVERIFY(reservation.isValid());
            QVERIFY(QFileInfo::exists(reservation.pendingPath));
            finals.insert(reservation.finalPath);
            pendings.insert(reservation.pendingPath);
        }
        QCOMPARE(finals.size(), qsizetype(kThreads));
        QCOMPARE(pendings.size(), qsizetype(kThreads));
    }

    void anUnfinishedCaptureIsNeverVisibleUnderItsFinalName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const CapturePublisher::Reservation reservation = CapturePublisher::reserve(
            dir.path(), QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QVERIFY(reservation.isValid());
        // Half-written: bytes on disk, but nothing under the final name.
        {
            QFile pending(reservation.pendingPath);
            QVERIFY(pending.open(QIODevice::WriteOnly));
            QCOMPARE(pending.write("partial-image-bytes"), 19);
        }
        QVERIFY(!QFileInfo::exists(reservation.finalPath));
        // And the reserved name really is reserved.
        QVERIFY(reservation.pendingPath.endsWith(CapturePublisher::kPendingSuffix));

        QString error;
        QVERIFY2(CapturePublisher::publish(reservation, &error), qPrintable(error));
        QVERIFY(QFileInfo::exists(reservation.finalPath));
        QVERIFY(!QFileInfo::exists(reservation.pendingPath));
        QCOMPARE(QFileInfo(reservation.finalPath).size(), qint64(19));
    }

    void aFailedEncodeLeavesNoCaptureBehind()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const CapturePublisher::Reservation reservation = CapturePublisher::reserve(
            dir.path(), QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QVERIFY(reservation.isValid());
        CapturePublisher::discard(reservation);
        QVERIFY(!QFileInfo::exists(reservation.pendingPath));
        QVERIFY(!QFileInfo::exists(reservation.finalPath));
        // The name is free again, so the next capture reuses it rather than
        // leaving a gap in the numbering.
        const CapturePublisher::Reservation again = CapturePublisher::reserve(
            dir.path(), QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QCOMPARE(again.finalPath, reservation.finalPath);
    }

    void neverReservesANameACaptureAlreadyOccupies()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const CapturePublisher::Reservation first = CapturePublisher::reserve(
            dir.path(), QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QVERIFY(first.isValid());
        QVERIFY(CapturePublisher::publish(first));

        const CapturePublisher::Reservation second = CapturePublisher::reserve(
            dir.path(), QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QVERIFY(second.isValid());
        QVERIFY(second.finalPath != first.finalPath);
        QVERIFY(QFileInfo::exists(first.finalPath));
    }

    void sweepRemovesOnlyAbandonedFiles()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("Some Game/Screenshots")));
        const QString nested = dir.path() + QStringLiteral("/Some Game/Screenshots");

        const CapturePublisher::Reservation running = CapturePublisher::reserve(
            nested, QStringLiteral("yyyy-MM-dd_HH-mm-ss"), QStringLiteral(".png"));
        QVERIFY(running.isValid());

        // A leftover from a crash: same shape, but old.
        const QString abandoned = nested + QStringLiteral("/2020-01-01_00-00-00.png")
            + CapturePublisher::kPendingSuffix;
        {
            QFile file(abandoned);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("stale");
            // setFileTime needs the handle, so age it before closing.
            QVERIFY(file.setFileTime(QDateTime::currentDateTime().addSecs(-3600),
                                     QFileDevice::FileModificationTime));
        }

        // A real capture next to them must not be touched.
        const QString keeper = nested + QStringLiteral("/keep-me.png");
        {
            QFile file(keeper);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("image");
        }

        QCOMPARE(CapturePublisher::sweepStale(dir.path(), 600), 1);
        QVERIFY(!QFileInfo::exists(abandoned));
        QVERIFY(QFileInfo::exists(running.pendingPath));   // too young to be abandoned
        QVERIFY(QFileInfo::exists(keeper));
    }
};

QTEST_GUILESS_MAIN(TestCapturePublisher)
#include "tst_capturepublisher.moc"
