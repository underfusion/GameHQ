#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantList>

// Strict parser for the small, bundled release-notes document shown by the
// desktop About modal. It emits only plain strings; QML never receives HTML.
class ReleaseNotes
{
public:
    static ReleaseNotes fromJson(const QByteArray& json, QString* error = nullptr);
    static ReleaseNotes loadBundled();

    bool isValid() const { return !m_version.isEmpty() && !m_sections.isEmpty(); }
    QString version() const { return m_version; }
    QVariantList sections() const { return m_sections; }

private:
    QString m_version;
    QVariantList m_sections;
};
