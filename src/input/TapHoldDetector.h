#pragma once
#include <QElapsedTimer>
#include <QObject>

class QTimer;

// Turns a single button's press/release into a tap OR a hold, never both
// (docs/controller-input.md). Released before the threshold → tapped();
// held past the threshold → held() (once) and the pending release is consumed.
// Pure logic, no UI deps — unit-testable.
class TapHoldDetector : public QObject
{
    Q_OBJECT
public:
    explicit TapHoldDetector(QObject* parent = nullptr);

    void setHoldThresholdMs(int ms);   // config input.share_hold_ms, default 2000

    void press();
    void release();
    void cancel();

signals:
    void tapped();
    void held();

private:
    QTimer* m_timer;
    QElapsedTimer m_pressClock;   // measures actual press→held/release time, for diagnostics
    int m_thresholdMs = 2000;
    bool m_pressed = false;
    bool m_consumed = false;   // hold already fired for this press
};
