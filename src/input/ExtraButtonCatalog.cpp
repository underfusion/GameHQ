#include "input/ExtraButtonCatalog.h"

#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QCryptographicHash>
#include <QSet>

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

bool ExtraButtonCatalog::isStandardControlLabel(const QString& label)
{
    QString normalized;
    normalized.reserve(label.size());
    for (const QChar character : label) {
        if (character.isLetterOrNumber())
            normalized.append(character.toLower());
    }
    static const QSet<QString> kStandardNames = {
        // Face buttons across families.
        QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"),
        QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"),
        QStringLiteral("cross"), QStringLiteral("circle"),
        QStringLiteral("square"), QStringLiteral("triangle"),
        // Menu / view family.
        QStringLiteral("menu"), QStringLiteral("start"), QStringLiteral("options"),
        QStringLiteral("view"), QStringLiteral("back"), QStringLiteral("select"),
        // D-pad.
        QStringLiteral("dpadup"), QStringLiteral("dpaddown"),
        QStringLiteral("dpadleft"), QStringLiteral("dpadright"),
        // Shoulders / triggers / thumbstick clicks.
        QStringLiteral("leftshoulder"), QStringLiteral("rightshoulder"),
        QStringLiteral("lb"), QStringLiteral("rb"),
        QStringLiteral("l1"), QStringLiteral("r1"),
        QStringLiteral("lefttrigger"), QStringLiteral("righttrigger"),
        QStringLiteral("lefttriggerbutton"), QStringLiteral("righttriggerbutton"),
        QStringLiteral("lt"), QStringLiteral("rt"),
        QStringLiteral("l2"), QStringLiteral("r2"),
        QStringLiteral("leftthumbstick"), QStringLiteral("rightthumbstick"),
        QStringLiteral("leftstick"), QStringLiteral("rightstick"),
        QStringLiteral("ls"), QStringLiteral("rs"),
        QStringLiteral("l3"), QStringLiteral("r3"),
        // System buttons (already routed through the system path).
        QStringLiteral("guide"), QStringLiteral("home"), QStringLiteral("xbox"),
        QStringLiteral("ps"), QStringLiteral("share"), QStringLiteral("capture"),
        QStringLiteral("create"),
    };
    return kStandardNames.contains(normalized);
}

ExtraButtonCatalog::Layout ExtraButtonCatalog::observe(
    const QString& logicalId, int buttonCount, const QStringList& reportedLabels)
{
    const QString signature = signatureFor(buttonCount, reportedLabels);
    // Hot path: an unchanged layout is answered from memory with zero
    // database reads or writes.
    if (const auto cached = m_cache.constFind(logicalId);
        cached != m_cache.cend() && cached->signature == signature)
        return *cached;

    Layout result;
    result.signature = signature;
    for (int index = 0; index < buttonCount; ++index) {
        const QString reported = index < reportedLabels.size() ? reportedLabels.at(index) : QString();
        result.labels.push_back(reported.isEmpty()
            ? QStringLiteral("Extra Button %1").arg(index + 1) : reported);
        result.controlIds.push_back(isStandardControlLabel(reported)
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
