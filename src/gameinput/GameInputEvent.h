#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ModernInput {

enum class GameInputEventKind {
    DeviceAdded,
    DeviceRemoved,
    CapabilityChanged,
    Sleep,
    Wake,
    SystemButtonPressed,
    SystemButtonReleased,
    Reading,
    RecoveryRequired
};

struct GameInputDeviceDescriptor
{
    QString deviceId;
    QString rootId;
    QString containerId;
    QString displayName;
    quint16 vendorId = 0;
    quint16 productId = 0;
    quint32 supportedInput = 0;
    quint32 supportedSystemButtons = 0;
    quint32 extraButtonCount = 0;
    QStringList buttonLabels;
};

// Callback threads publish only this immutable value object. It deliberately
// contains no QObject, COM pointer, PnP path or serial-number-bearing field.
struct GameInputEvent
{
    GameInputEventKind kind = GameInputEventKind::Reading;
    quint64 sequence = 0;
    quint64 timestamp = 0;
    QString deviceId;
    QString controlId;
    GameInputDeviceDescriptor device;
    quint32 standardButtons = 0;
    QVector<quint8> buttonStates;

    bool isCoalescibleState() const
    {
        return kind == GameInputEventKind::Reading;
    }

    bool isDiscrete() const
    {
        return !isCoalescibleState();
    }
};

struct GameInputEventBatch
{
    QVector<GameInputEvent> events;
    bool forceResync = false;
    QStringList uncertainDevices;
    quint64 overflowCount = 0;
};

} // namespace ModernInput

Q_DECLARE_METATYPE(ModernInput::GameInputEvent)
Q_DECLARE_METATYPE(ModernInput::GameInputEventBatch)

