#include "input/ControllerIdentity.h"

#include <QCryptographicHash>
#include <QSet>

namespace ControllerIdentity
{
QString legacySlotFingerprint(int slot)
{
    return QStringLiteral("xinput.slot%1").arg(slot);
}

bool isLegacySlotFingerprint(const QString& fingerprint)
{
    return fingerprint.startsWith(QLatin1String("xinput.slot"));
}

QString endpointFingerprint(const QString& devicePath)
{
    if (devicePath.isEmpty())
        return {};
    const QByteArray digest = QCryptographicHash::hash(
        devicePath.toLower().toUtf8(), QCryptographicHash::Sha256).toHex().left(20);
    return QStringLiteral("hid.endpoint.%1").arg(QString::fromLatin1(digest));
}

QString resolveXInputFingerprint(const QStringList& xinputClassIdentities,
                                 int connectedSlotCount, int slot)
{
    QSet<QString> distinct;
    for (const QString& identity : xinputClassIdentities) {
        if (!identity.isEmpty())
            distinct.insert(identity);
    }
    // Only a one-device, one-slot world is unambiguous. Everything else keeps
    // the legacy per-slot key — a wrong stable identity would be worse than
    // an honest slot-scoped one.
    if (distinct.size() == 1 && connectedSlotCount == 1)
        return *distinct.cbegin();
    return legacySlotFingerprint(slot);
}
} // namespace ControllerIdentity
