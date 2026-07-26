#include <QtTest>

#include "input/Gamepad.h"
#include "input/SonyReportLayout.h"
#include "input/StickNav.h"

class SonyReportLayoutTest : public QObject
{
    Q_OBJECT

private slots:
    void resolvesUsbAndBluetoothStickOffsets()
    {
        using Family = SonyReportLayout::Family;
        QCOMPARE(SonyReportLayout::stickAxisBase(Family::Ds4, 5), 1);
        QCOMPARE(SonyReportLayout::stickAxisBase(Family::Ds4, 7), 3);
        QCOMPARE(SonyReportLayout::stickAxisBase(Family::DualSense, 8), 1);
        QCOMPARE(SonyReportLayout::stickAxisBase(Family::DualSense, 10), 3);
    }

    void bluetoothDs4NeutralStickDoesNotBecomeLeft()
    {
        // Report 0x11: bytes 3/4 are LX/LY. Byte 1 is transport metadata and
        // may be zero; treating it as LX was the source of permanent Left.
        const unsigned char report[] = {0x11, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x08};
        const int axisBase = SonyReportLayout::stickAxisBase(
            SonyReportLayout::Family::Ds4, 7);
        constexpr StickNav::AxisConfig nav{128, 60, 30, false};
        QCOMPARE(StickNav::bits(nav, report[axisBase], report[axisBase + 1]), quint32(0));
        QVERIFY(StickNav::bits(nav, report[1], report[2])
                & (quint32(1) << Gamepad::DpadLeft));
    }
};

QTEST_APPLESS_MAIN(SonyReportLayoutTest)
#include "tst_sonyreportlayout.moc"
