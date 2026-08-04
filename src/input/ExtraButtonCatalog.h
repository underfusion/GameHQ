#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class CaptureDatabase;

namespace ModernInput {

class ExtraButtonCatalog
{
public:
    struct Layout {
        QString signature;
        QStringList controlIds;
        QStringList labels;
        bool changed = false;
        bool needsReconfirmation = false;
    };

    explicit ExtraButtonCatalog(CaptureDatabase* database = nullptr)
        : m_database(database) {}

    Layout observe(const QString& logicalId, int buttonCount,
                   const QStringList& reportedLabels);
    bool confirm(const QString& logicalId);
    static QString signatureFor(int buttonCount, const QStringList& labels);

private:
    CaptureDatabase* m_database = nullptr;
    // observe() runs per delivered reading; the database is touched only when
    // the layout signature or reconfirmation state actually changes.
    QHash<QString, Layout> m_cache;
};

} // namespace ModernInput
