#include "input/ControlId.h"

#include <QStringList>
#include <QCryptographicHash>

namespace ControlId {

QString genericButton(int index)
{
    return QStringLiteral("gamepad.button.%1").arg(index);
}

QString deviceButton(const QString& logicalId, const QString& layoutSignature, int index)
{
    return QStringLiteral("gamepad.device.%1.layout.%2.button.%3")
        .arg(logicalId, layoutSignature).arg(index);
}

bool isGenericButton(const QString& code)
{
    return code.startsWith(QStringLiteral("gamepad.button."));
}

bool isDeviceButton(const QString& code)
{
    const QStringList parts = code.split(QLatin1Char('.'));
    if (parts.size() != 7 || parts.at(0) != QLatin1String("gamepad")
        || parts.at(1) != QLatin1String("device") || parts.at(2).isEmpty()
        || parts.at(3) != QLatin1String("layout") || parts.at(4).isEmpty()
        || parts.at(5) != QLatin1String("button"))
        return false;
    bool numeric = false;
    const int index = parts.at(6).toInt(&numeric);
    return numeric && index >= 0;
}

QString rawHidUsage(const QString& deviceIdentity, quint16 usagePage, quint16 usage)
{
    const QString anonymous = QString::fromLatin1(QCryptographicHash::hash(
        deviceIdentity.toUtf8(), QCryptographicHash::Sha256).toHex().left(16));
    return QStringLiteral("gamepad.raw.%1.usage.%2.%3")
        .arg(anonymous).arg(usagePage, 0, 16).arg(usage, 0, 16);
}

bool isRawHidUsage(const QString& code)
{
    const QStringList parts = code.split(QLatin1Char('.'));
    if (parts.size() != 6 || parts.at(0) != QLatin1String("gamepad")
        || parts.at(1) != QLatin1String("raw") || parts.at(2).isEmpty()
        || parts.at(3) != QLatin1String("usage"))
        return false;
    bool pageOk = false;
    bool usageOk = false;
    parts.at(4).toUInt(&pageOk, 16);
    parts.at(5).toUInt(&usageOk, 16);
    return pageOk && usageOk;
}

bool isCanonical(const QString& code)
{
    if (isDeviceButton(code))
        return true;
    if (isRawHidUsage(code))
        return true;
    if (isGenericButton(code)) {
        bool numeric = false;
        const int index = code.mid(QStringLiteral("gamepad.button.").size()).toInt(&numeric);
        return numeric && index >= 0;
    }
    return code == FaceSouth || code == FaceEast || code == FaceNorth || code == FaceWest
        || code == FaceC || code == FaceZ
        || code == ShoulderLeft || code == ShoulderRight
        || code == TriggerLeft || code == TriggerRight
        || code == ThumbLeft || code == ThumbRight
        || code == DpadUp || code == DpadDown || code == DpadLeft || code == DpadRight
        || code == PaddleLeft1 || code == PaddleLeft2
        || code == PaddleRight1 || code == PaddleRight2
        || code == StickRightUp || code == StickRightDown
        || code == Menu || code == Guide || code == Capture || code == ViewBack;
}

QString label(const QString& code, ControllerFamily family)
{
    if (isDeviceButton(code)) {
        bool numeric = false;
        const int index = code.section(QLatin1Char('.'), -1).toInt(&numeric);
        return numeric ? QStringLiteral("Extra Button %1").arg(index + 1)
                       : QStringLiteral("Extra Button");
    }
    if (isRawHidUsage(code)) {
        return QStringLiteral("Raw HID %1:%2")
            .arg(code.section(QLatin1Char('.'), -2, -2).toUpper(),
                 code.section(QLatin1Char('.'), -1).toUpper());
    }
    if (isGenericButton(code))
        return QStringLiteral("Button %1").arg(code.section(QLatin1Char('.'), -1));

    switch (family) {
    case ControllerFamily::PlayStation:
        if (code == FaceSouth)     return QStringLiteral("Cross");
        if (code == FaceEast)      return QStringLiteral("Circle");
        if (code == FaceNorth)     return QStringLiteral("Triangle");
        if (code == FaceWest)      return QStringLiteral("Square");
        if (code == ShoulderLeft)  return QStringLiteral("L1");
        if (code == ShoulderRight) return QStringLiteral("R1");
        if (code == TriggerLeft)   return QStringLiteral("L2");
        if (code == TriggerRight)  return QStringLiteral("R2");
        if (code == Menu)          return QStringLiteral("Options");
        if (code == Guide)         return QStringLiteral("PS");
        if (code == Capture)       return QStringLiteral("Share");
        if (code == ViewBack)      return QStringLiteral("View / Back");
        if (code == ThumbLeft)     return QStringLiteral("L3");
        if (code == ThumbRight)    return QStringLiteral("R3");
        break;
    case ControllerFamily::Xbox:
        if (code == FaceSouth)     return QStringLiteral("A");
        if (code == FaceEast)      return QStringLiteral("B");
        if (code == FaceNorth)     return QStringLiteral("Y");
        if (code == FaceWest)      return QStringLiteral("X");
        if (code == ShoulderLeft)  return QStringLiteral("LB");
        if (code == ShoulderRight) return QStringLiteral("RB");
        if (code == TriggerLeft)   return QStringLiteral("LT");
        if (code == TriggerRight)  return QStringLiteral("RT");
        if (code == Menu)          return QStringLiteral("Menu");
        if (code == Guide)         return QStringLiteral("Guide");
        if (code == Capture)       return QStringLiteral("System Share");
        if (code == ViewBack)      return QStringLiteral("View / Back");
        if (code == ThumbLeft)     return QStringLiteral("LS Click");
        if (code == ThumbRight)    return QStringLiteral("RS Click");
        break;
    case ControllerFamily::Nintendo:
        // Face buttons are mirrored versus Xbox at the same physical positions.
        if (code == FaceSouth)     return QStringLiteral("B");
        if (code == FaceEast)      return QStringLiteral("A");
        if (code == FaceNorth)     return QStringLiteral("X");
        if (code == FaceWest)      return QStringLiteral("Y");
        if (code == ShoulderLeft)  return QStringLiteral("L");
        if (code == ShoulderRight) return QStringLiteral("R");
        if (code == TriggerLeft)   return QStringLiteral("ZL");
        if (code == TriggerRight)  return QStringLiteral("ZR");
        if (code == Menu)          return QStringLiteral("+");
        if (code == Guide)         return QStringLiteral("Home");
        if (code == Capture)       return QStringLiteral("Capture");
        if (code == ViewBack)      return QStringLiteral("View / Back");
        break;
    case ControllerFamily::Generic:
        break;
    }

    if (code == FaceSouth)     return QStringLiteral("South Button");
    if (code == FaceEast)      return QStringLiteral("East Button");
    if (code == FaceNorth)     return QStringLiteral("North Button");
    if (code == FaceWest)      return QStringLiteral("West Button");
    if (code == ShoulderLeft)  return QStringLiteral("L1");
    if (code == ShoulderRight) return QStringLiteral("R1");
    if (code == TriggerLeft)   return QStringLiteral("L2");
    if (code == TriggerRight)  return QStringLiteral("R2");
    if (code == DpadUp)        return QStringLiteral("D-Up");
    if (code == DpadDown)      return QStringLiteral("D-Down");
    if (code == DpadLeft)      return QStringLiteral("D-Left");
    if (code == DpadRight)     return QStringLiteral("D-Right");
    if (code == Menu)          return QStringLiteral("Menu");
    if (code == Guide)         return QStringLiteral("Guide");
    if (code == Capture)       return QStringLiteral("Capture");
    if (code == ViewBack)      return QStringLiteral("View / Back");
    if (code == ThumbLeft)     return QStringLiteral("Left Stick Click");
    if (code == ThumbRight)    return QStringLiteral("Right Stick Click");
    if (code == FaceC)         return QStringLiteral("C");
    if (code == FaceZ)         return QStringLiteral("Z");
    if (code == PaddleLeft1)   return QStringLiteral("Left Paddle 1");
    if (code == PaddleLeft2)   return QStringLiteral("Left Paddle 2");
    if (code == PaddleRight1)  return QStringLiteral("Right Paddle 1");
    if (code == PaddleRight2)  return QStringLiteral("Right Paddle 2");
    if (code == StickRightUp)   return QStringLiteral("Right Stick Up");
    if (code == StickRightDown) return QStringLiteral("Right Stick Down");
    return QStringLiteral("?");
}

} // namespace ControlId
