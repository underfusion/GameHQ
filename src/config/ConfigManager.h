#pragma once
#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVariant>

// JSON config (config.json). Flat keys with dotted namespaces, e.g. "capture.mode".
// Unknown keys are preserved on save so future versions stay compatible.
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QString filePath, QObject* parent = nullptr);

    bool load();   // missing file is fine → defaults
    bool save() const;

    QVariant value(const QString& key, const QVariant& fallback = {}) const;
    void setValue(const QString& key, const QVariant& value);
    QVariant defaultValue(const QString& key, const QVariant& fallback = {}) const;
    bool isDefault(const QString& key) const;
    bool resetValue(const QString& key);
    bool resetGroup(const QString& prefix);
    bool resetAll();

signals:
    void valueChanged(const QString& key, const QVariant& value);
    void groupReset(const QString& prefix);

private:
    static QJsonObject defaults();

    QString m_filePath;
    QJsonObject m_overrides;
};
