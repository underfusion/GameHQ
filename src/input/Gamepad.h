#pragma once
#include "input/ControlId.h"

#include <QObject>
#include <QString>

// Abstract gamepad backend (docs/controller-input.md). DualSense-first, but the
// interface lets XInput/SDL backends be added later. Buttons are logical, not
// device-specific; the concrete backend maps raw reports onto this enum.
class Gamepad : public QObject
{
    Q_OBJECT
public:
    enum Button {
        Share, Options, PS,
        Cross, Circle, Triangle, Square,
        L1, R1,
        DpadUp, DpadDown, DpadLeft, DpadRight,
        ButtonCount
    };
    Q_ENUM(Button)

    // Indices >= GenericButtonBase are backend-reported buttons with no known
    // canonical position (see docs/controller-input.md): still bindable,
    // just labeled "Button N" instead of a named position. The quint32 edge
    // bitmask backends already build caps this range at 32 bits.
    static constexpr int GenericButtonBase = ButtonCount;
    static constexpr int MaxButtons = 32;

    explicit Gamepad(QObject* parent = nullptr) : QObject(parent) {}
    ~Gamepad() override = default;

    // Begin listening for input. Returns false only on a hard setup failure;
    // "no pad connected" is NOT a failure (returns true, emits nothing).
    virtual bool start() = 0;

    // Device metadata for the pad currently driving input on this backend.
    // Base implementation is a safe empty/generic default; backends that can
    // identify their active device (VID/PID, layout) override this.
    virtual ControlId::DeviceProfile profile() const { return {}; }

    static QString buttonName(int b)
    {
        switch (b) {
        case Share:     return QStringLiteral("Share");
        case Options:   return QStringLiteral("Options");
        case PS:        return QStringLiteral("PS");
        case Cross:     return QStringLiteral("Cross");
        case Circle:    return QStringLiteral("Circle");
        case Triangle:  return QStringLiteral("Triangle");
        case Square:    return QStringLiteral("Square");
        case L1:        return QStringLiteral("L1");
        case R1:        return QStringLiteral("R1");
        case DpadUp:    return QStringLiteral("D-Up");
        case DpadDown:  return QStringLiteral("D-Down");
        case DpadLeft:  return QStringLiteral("D-Left");
        case DpadRight: return QStringLiteral("D-Right");
        default:
            // Matches ControlId::label()'s numbering for the same generic code.
            return b >= GenericButtonBase
                ? QStringLiteral("Button %1").arg(b - GenericButtonBase)
                : QStringLiteral("?");
        }
    }

    // Canonical, device-neutral code for a button index — what bindings
    // persist. Positions map physically (Cross/A/B share "face_south"); an
    // index with no known position falls back to a stable per-device generic
    // code so unmapped WinMM/XInput buttons stay bindable.
    static QString controlIdFor(int b)
    {
        switch (b) {
        case Cross:     return ControlId::FaceSouth;
        case Circle:    return ControlId::FaceEast;
        case Triangle:  return ControlId::FaceNorth;
        case Square:    return ControlId::FaceWest;
        case L1:        return ControlId::ShoulderLeft;
        case R1:        return ControlId::ShoulderRight;
        case DpadUp:    return ControlId::DpadUp;
        case DpadDown:  return ControlId::DpadDown;
        case DpadLeft:  return ControlId::DpadLeft;
        case DpadRight: return ControlId::DpadRight;
        case Options:   return ControlId::Menu;
        case PS:        return ControlId::Guide;
        case Share:     return ControlId::Capture;
        default:
            return b >= GenericButtonBase
                ? ControlId::genericButton(b - GenericButtonBase)
                : QString();
        }
    }

signals:
    // Canonical backend events consumed by the binding runtime. The legacy
    // integer signals remain available during the incremental migration.
    void controlPressed(const QString& controlId, int family,
                        const QString& backend, const QString& fingerprint,
                        const QString& displayName, int legacyButton);
    void controlReleased(const QString& controlId, int family,
                         const QString& backend, const QString& fingerprint,
                         const QString& displayName, int legacyButton);
    void buttonPressed(int button);
    void buttonReleased(int button);
    void connected(bool isConnected);

protected:
    void publishButtonPressed(int button)
    {
        const auto device = profile();
        emit controlPressed(controlIdFor(button), static_cast<int>(device.family),
                            device.backend, device.fingerprint, device.displayName, button);
        emit buttonPressed(button);
    }

    void publishButtonReleased(int button)
    {
        const auto device = profile();
        emit controlReleased(controlIdFor(button), static_cast<int>(device.family),
                             device.backend, device.fingerprint, device.displayName, button);
        emit buttonReleased(button);
    }
};
