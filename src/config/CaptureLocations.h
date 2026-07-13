#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class ConfigManager;

class CaptureLocations : public QObject
{
    Q_OBJECT
public:
    enum class Kind { Screenshots, Clips };

    explicit CaptureLocations(ConfigManager* config, QObject* parent = nullptr);

    QString screenshotsBaseRoot() const;
    QString clipsBaseRoot() const;
    QString screenshotDir(const QString& gameName) const;
    QString clipsDir(const QString& gameName) const;
    QStringList managedRoots() const;

    bool setBaseRoot(Kind kind, const QString& path, QString* error = nullptr);
    bool resetBaseRoot(Kind kind, QString* error = nullptr);
    void preserveCurrentRoots();

signals:
    void locationsChanged();

private:
    QString configuredRoot(const QString& key) const;
    QString keyFor(Kind kind) const;
    void rememberPreviousRoot(const QString& root);

    ConfigManager* m_config;
};
