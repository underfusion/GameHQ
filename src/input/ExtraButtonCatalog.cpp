#include "input/ExtraButtonCatalog.h"

#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QCryptographicHash>

namespace ModernInput {

QString ExtraButtonCatalog::signatureFor(int buttonCount, const QStringList& labels)
{
    QByteArray material = QByteArray::number(buttonCount);
    for (int index = 0; index < buttonCount; ++index) {
        material.append('\0');
        material.append(index < labels.size() ? labels.at(index).toUtf8() : QByteArray());
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(material, QCryptographicHash::Sha256).toHex().left(16));
}

ExtraButtonCatalog::Layout ExtraButtonCatalog::observe(
    const QString& logicalId, int buttonCount, const QStringList& reportedLabels)
{
    Layout result;
    result.signature = signatureFor(buttonCount, reportedLabels);
    for (int index = 0; index < buttonCount; ++index) {
        const QString reported = index < reportedLabels.size() ? reportedLabels.at(index) : QString();
        result.labels.push_back(reported.isEmpty()
            ? QStringLiteral("Extra Button %1").arg(index + 1) : reported);
        result.controlIds.push_back(ControlId::deviceButton(logicalId, result.signature, index));
    }

    if (!m_database || logicalId.isEmpty())
        return result;
    const ControllerLayoutRow previous = m_database->controllerLayout(logicalId);
    result.changed = !previous.logicalId.isEmpty()
        && previous.layoutSignature != result.signature;
    result.needsReconfirmation = result.changed || previous.needsReconfirmation;
    m_database->upsertControllerLayout(
        {logicalId, result.signature, result.labels, result.needsReconfirmation});
    return result;
}

bool ExtraButtonCatalog::confirm(const QString& logicalId)
{
    return !m_database || m_database->confirmControllerLayout(logicalId);
}

} // namespace ModernInput
