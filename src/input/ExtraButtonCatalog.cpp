#include "input/ExtraButtonCatalog.h"

#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QCryptographicHash>

namespace ModernInput {

QString ExtraButtonCatalog::signatureFor(
    int buttonCount, const QVector<GameInputButtonDescriptor>& buttons)
{
    QByteArray material = QByteArray::number(buttonCount);
    for (int index = 0; index < buttonCount; ++index) {
        const GameInputButtonDescriptor descriptor = index < buttons.size()
            ? buttons.at(index) : GameInputButtonDescriptor{};
        material.append('\0');
        material.append(QByteArray::number(descriptor.rawLabel));
        material.append(':');
        material.append(QByteArray::number(int(descriptor.classification)));
        material.append(':');
        material.append(descriptor.normalizedLabel.toUtf8());
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(material, QCryptographicHash::Sha256).toHex().left(16));
}

ExtraButtonCatalog::Layout ExtraButtonCatalog::observe(
    const QString& logicalId, int buttonCount,
    const QVector<GameInputButtonDescriptor>& buttons)
{
    const QString signature = signatureFor(buttonCount, buttons);
    // Hot path: an unchanged layout is answered from memory with zero
    // database reads or writes.
    if (const auto cached = m_cache.constFind(logicalId);
        cached != m_cache.cend() && cached->signature == signature)
        return *cached;

    Layout result;
    result.signature = signature;
    for (int index = 0; index < buttonCount; ++index) {
        const GameInputButtonDescriptor descriptor = index < buttons.size()
            ? buttons.at(index) : GameInputButtonDescriptor{};
        const QString reported = descriptor.normalizedLabel;
        result.labels.push_back(reported.isEmpty()
            ? QStringLiteral("Extra Button %1").arg(index + 1) : reported);
        const bool canonical = descriptor.classification
                == GameInputButtonClassification::Standard
            || descriptor.classification == GameInputButtonClassification::System;
        result.controlIds.push_back(canonical
            ? QString()
            : ControlId::deviceButton(logicalId, result.signature, index));
    }

    if (!m_database || logicalId.isEmpty())
        return result;
    const ControllerLayoutRow previous = m_database->controllerLayout(logicalId);
    result.changed = !previous.logicalId.isEmpty()
        && previous.layoutSignature != result.signature;
    result.needsReconfirmation = result.changed || previous.needsReconfirmation;
    if (previous.logicalId.isEmpty() || result.changed
        || previous.needsReconfirmation != result.needsReconfirmation) {
        m_database->upsertControllerLayout(
            {logicalId, result.signature, result.labels, result.needsReconfirmation});
    }
    // `changed` is a transition flag: it must fire once per signature change,
    // not once per reading, so the cached copy reports stable state.
    Layout cachedCopy = result;
    cachedCopy.changed = false;
    m_cache.insert(logicalId, cachedCopy);
    return result;
}

bool ExtraButtonCatalog::confirm(const QString& logicalId)
{
    m_cache.remove(logicalId);
    return !m_database || m_database->confirmControllerLayout(logicalId);
}

} // namespace ModernInput
