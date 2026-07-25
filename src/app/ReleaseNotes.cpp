#include "app/ReleaseNotes.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QVariantMap>

namespace
{
constexpr int kMaxSections = 8;
constexpr int kMaxItemsPerSection = 20;
constexpr int kMaxTitleLength = 80;
constexpr int kMaxItemLength = 500;

void setError(QString* error, const QString& value)
{
    if (error)
        *error = value;
}
}

ReleaseNotes ReleaseNotes::fromJson(const QByteArray& json, QString* error)
{
    ReleaseNotes result;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QStringLiteral("Release notes are not a valid JSON object."));
        return result;
    }

    const QJsonObject root = document.object();
    const QString version = root.value(QStringLiteral("version")).toString().trimmed();
    static const QRegularExpression versionPattern(QStringLiteral(R"(^\d+\.\d+\.\d+$)"));
    if (!versionPattern.match(version).hasMatch()) {
        setError(error, QStringLiteral("Release notes contain an invalid version."));
        return result;
    }

    const QJsonValue sectionsValue = root.value(QStringLiteral("sections"));
    if (!sectionsValue.isArray()) {
        setError(error, QStringLiteral("Release notes sections must be an array."));
        return result;
    }
    const QJsonArray sections = sectionsValue.toArray();
    if (sections.isEmpty() || sections.size() > kMaxSections) {
        setError(error, QStringLiteral("Release notes contain an invalid section count."));
        return result;
    }

    QVariantList parsedSections;
    for (const QJsonValue& sectionValue : sections) {
        if (!sectionValue.isObject()) {
            setError(error, QStringLiteral("Every release-notes section must be an object."));
            return {};
        }
        const QJsonObject section = sectionValue.toObject();
        const QString title = section.value(QStringLiteral("title")).toString().trimmed();
        const QJsonValue itemsValue = section.value(QStringLiteral("items"));
        if (title.isEmpty() || title.size() > kMaxTitleLength || !itemsValue.isArray()) {
            setError(error, QStringLiteral("A release-notes section is malformed."));
            return {};
        }

        const QJsonArray items = itemsValue.toArray();
        if (items.isEmpty() || items.size() > kMaxItemsPerSection) {
            setError(error, QStringLiteral("A release-notes section has an invalid item count."));
            return {};
        }
        QStringList parsedItems;
        for (const QJsonValue& itemValue : items) {
            if (!itemValue.isString()) {
                setError(error, QStringLiteral("Every release-note item must be plain text."));
                return {};
            }
            const QString item = itemValue.toString().trimmed();
            if (item.isEmpty() || item.size() > kMaxItemLength) {
                setError(error, QStringLiteral("A release-note item has an invalid length."));
                return {};
            }
            parsedItems.append(item);
        }

        parsedSections.append(QVariantMap{
            { QStringLiteral("title"), title },
            { QStringLiteral("items"), parsedItems },
        });
    }

    result.m_version = version;
    result.m_sections = parsedSections;
    setError(error, {});
    return result;
}

ReleaseNotes ReleaseNotes::loadBundled()
{
    QFile file(QStringLiteral(":/release-notes/release-notes.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Could not open bundled release notes");
        return {};
    }

    QString error;
    ReleaseNotes notes = fromJson(file.readAll(), &error);
    if (!notes.isValid())
        qWarning("Could not parse bundled release notes: %s", qPrintable(error));
    return notes;
}
