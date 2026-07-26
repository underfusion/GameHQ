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
constexpr int kMaxMarkdownLength = 64 * 1024;
constexpr int kMaxMarkdownBlocks = 128;
constexpr int kMaxMarkdownBlockLength = 2000;

void setError(QString* error, const QString& value)
{
    if (error)
        *error = value;
}

QString plainInlineMarkdown(QString text)
{
    static const QRegularExpression image(
        QStringLiteral(R"(!\[([^\]]*)\]\([^)]+\))"));
    static const QRegularExpression link(
        QStringLiteral(R"(\[([^\]]+)\]\([^)]+\))"));
    static const QRegularExpression strongAsterisk(
        QStringLiteral(R"(\*\*([^*]+)\*\*)"));
    static const QRegularExpression strongUnderscore(
        QStringLiteral(R"(__([^_]+)__)"));
    static const QRegularExpression code(
        QStringLiteral(R"(`([^`]+)`)"));

    text.replace(image, QStringLiteral("\\1"));
    text.replace(link, QStringLiteral("\\1"));
    text.replace(strongAsterisk, QStringLiteral("\\1"));
    text.replace(strongUnderscore, QStringLiteral("\\1"));
    text.replace(code, QStringLiteral("\\1"));
    return text.trimmed().left(kMaxMarkdownBlockLength);
}

QVariantMap markdownBlock(const QString& kind, const QString& rawText)
{
    const QString text = plainInlineMarkdown(rawText);
    QString lead;
    QString body = text;

    if (kind == QStringLiteral("bullet")) {
        static const QRegularExpression strongLead(
            QStringLiteral(R"(^(?:\*\*|__)(.+?)(?:\*\*|__)\s*(.*)$)"));
        const QRegularExpressionMatch match = strongLead.match(rawText.trimmed());
        if (match.hasMatch()) {
            lead = plainInlineMarkdown(match.captured(1));
            body = plainInlineMarkdown(match.captured(2));
        }
    }

    return {
        { QStringLiteral("kind"), kind },
        { QStringLiteral("text"), text },
        { QStringLiteral("lead"), lead },
        { QStringLiteral("body"), body },
    };
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

QVariantList ReleaseNotes::blocksFromMarkdown(const QString& markdown)
{
    if (markdown.isEmpty())
        return {};

    const QString bounded = markdown.left(kMaxMarkdownLength);
    const QStringList lines = bounded.split(QRegularExpression(QStringLiteral(R"(\r?\n)")));
    static const QRegularExpression heading(
        QStringLiteral(R"(^\s*#{1,6}\s+(.+?)\s*#*\s*$)"));
    static const QRegularExpression bullet(
        QStringLiteral(R"(^\s*(?:[-*+]|\d+[.)])\s+(.+)$)"));

    QVariantList blocks;
    QStringList paragraph;
    auto appendBlock = [&blocks](const QVariantMap& block) {
        if (blocks.size() < kMaxMarkdownBlocks
            && !block.value(QStringLiteral("text")).toString().isEmpty()) {
            blocks.append(block);
        }
    };
    auto flushParagraph = [&]() {
        if (paragraph.isEmpty())
            return;
        appendBlock(markdownBlock(QStringLiteral("paragraph"),
                                  paragraph.join(QLatin1Char(' '))));
        paragraph.clear();
    };

    for (const QString& rawLine : lines) {
        if (blocks.size() >= kMaxMarkdownBlocks)
            break;

        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty()) {
            flushParagraph();
            continue;
        }

        const QRegularExpressionMatch headingMatch = heading.match(rawLine);
        if (headingMatch.hasMatch()) {
            flushParagraph();
            appendBlock(markdownBlock(QStringLiteral("heading"),
                                      headingMatch.captured(1)));
            continue;
        }

        const QRegularExpressionMatch bulletMatch = bullet.match(rawLine);
        if (bulletMatch.hasMatch()) {
            flushParagraph();
            appendBlock(markdownBlock(QStringLiteral("bullet"),
                                      bulletMatch.captured(1)));
            continue;
        }

        paragraph.append(trimmed);
    }
    flushParagraph();
    return blocks;
}
