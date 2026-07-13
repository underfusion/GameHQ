#include "input/ControlId.h"

namespace ControlId {

QString genericButton(int index)
{
    return QStringLiteral("gamepad.button.%1").arg(index);
}

bool isGenericButton(const QString& code)
{
    return code.startsWith(QStringLiteral("gamepad.button."));
}

QString label(const QString& code, ControllerFamily family)
{
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
        if (code == Menu)          return QStringLiteral("Options");
        if (code == Guide)         return QStringLiteral("PS");
        if (code == Capture)       return QStringLiteral("Share");
        break;
    case ControllerFamily::Xbox:
        if (code == FaceSouth)     return QStringLiteral("A");
        if (code == FaceEast)      return QStringLiteral("B");
        if (code == FaceNorth)     return QStringLiteral("Y");
        if (code == FaceWest)      return QStringLiteral("X");
        if (code == ShoulderLeft)  return QStringLiteral("LB");
        if (code == ShoulderRight) return QStringLiteral("RB");
        if (code == Menu)          return QStringLiteral("Menu");
        if (code == Guide)         return QStringLiteral("Guide");
        if (code == Capture)       return QStringLiteral("View");
        break;
    case ControllerFamily::Nintendo:
        // Face buttons are mirrored versus Xbox at the same physical positions.
        if (code == FaceSouth)     return QStringLiteral("B");
        if (code == FaceEast)      return QStringLiteral("A");
        if (code == FaceNorth)     return QStringLiteral("X");
        if (code == FaceWest)      return QStringLiteral("Y");
        if (code == ShoulderLeft)  return QStringLiteral("L");
        if (code == ShoulderRight) return QStringLiteral("R");
        if (code == Menu)          return QStringLiteral("+");
        if (code == Guide)         return QStringLiteral("Home");
        if (code == Capture)       return QStringLiteral("Capture");
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
    if (code == DpadUp)        return QStringLiteral("D-Up");
    if (code == DpadDown)      return QStringLiteral("D-Down");
    if (code == DpadLeft)      return QStringLiteral("D-Left");
    if (code == DpadRight)     return QStringLiteral("D-Right");
    if (code == Menu)          return QStringLiteral("Menu");
    if (code == Guide)         return QStringLiteral("Guide");
    if (code == Capture)       return QStringLiteral("Capture");
    return QStringLiteral("?");
}

} // namespace ControlId
