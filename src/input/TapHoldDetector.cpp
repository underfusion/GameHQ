#include "input/TapHoldDetector.h"

#include <QDebug>
#include <QTimer>

TapHoldDetector::TapHoldDetector(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, [this] {
        if (m_pressed) {
            m_consumed = true;   // release after this emits nothing
            qInfo() << "TapHold: held fired after" << m_pressClock.elapsed()
                     << "ms (threshold" << m_thresholdMs << "ms)";
            emit held();
        }
    });
}

void TapHoldDetector::setHoldThresholdMs(int ms)
{
    if (ms > 0)
        m_thresholdMs = ms;
}

void TapHoldDetector::press()
{
    if (m_pressed)
        return;                 // ignore auto-repeat / duplicate down
    m_pressed = true;
    m_consumed = false;
    m_pressClock.start();
    m_timer->start(m_thresholdMs);
}

void TapHoldDetector::release()
{
    if (!m_pressed)
        return;
    m_pressed = false;
    m_timer->stop();
    if (!m_consumed) {
        qInfo() << "TapHold: released as tap after" << m_pressClock.elapsed()
                 << "ms (threshold" << m_thresholdMs << "ms)";
        emit tapped();
    }
}

void TapHoldDetector::cancel()
{
    if (!m_pressed)
        return;
    m_pressed = false;
    m_consumed = true;
    m_timer->stop();
    qInfo() << "TapHold: cancelled after" << m_pressClock.elapsed() << "ms";
}
