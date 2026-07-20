#include <QtTest>

#include "core/UpdateMaintenance.h"

#include <QTemporaryDir>

#include <fstream>

class UpdateMaintenanceTest : public QObject
{
    Q_OBJECT
private slots:
    void markerLifecycleAndStaleRecovery();
};

void UpdateMaintenanceTest::markerLifecycleAndStaleRecovery()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::filesystem::path root = dir.path().toStdWString();
    const auto now = std::filesystem::file_time_type::clock::now();
    QCOMPARE(maintenance::inspect(root, false, now).state,
             maintenance::State::Inactive);

    std::string error;
    QVERIFY2(maintenance::begin(root, error), error.c_str());
    QCOMPARE(maintenance::inspect(root, false, now).state,
             maintenance::State::Active);

    const auto marker = root / L".update" / L"maintenance.lock";
    std::filesystem::last_write_time(marker, now - std::chrono::minutes(6));
    QCOMPARE(maintenance::inspect(root, true, now).state,
             maintenance::State::Active);
    QCOMPARE(maintenance::inspect(root, false, now).state,
             maintenance::State::StaleRecovery);

    std::ofstream(root / L".update" / L"transaction.phase") << "healthy\n";
    QCOMPARE(maintenance::inspect(root, false, now).state,
             maintenance::State::Inactive);
    maintenance::finish(root);
    QVERIFY(!std::filesystem::exists(marker));
}

QTEST_APPLESS_MAIN(UpdateMaintenanceTest)
#include "tst_updatemaintenance.moc"
