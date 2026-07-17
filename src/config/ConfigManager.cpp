#include "config/ConfigManager.h"
#include "config/ConfigKeys.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>
#include <QDebug>

ConfigManager::ConfigManager(QString filePath, QObject* parent)
    : QObject(parent)
    , m_filePath(std::move(filePath))
{
}

QJsonObject ConfigManager::defaults()
{
    return {
        { ConfigKeys::CaptureMode,             "only_in_games" }, // only_in_games | whitelist | always
        { ConfigKeys::CaptureScreenshotFormat, "png" },           // png | jpg
        { ConfigKeys::CaptureJpegQuality,      90 },              // 1-100, only used when format=jpg
        { ConfigKeys::CaptureScreenshotSound,  true },
        { ConfigKeys::CaptureScreenshotNotify, true },
        { ConfigKeys::ReplayClipSound,         true },
        { ConfigKeys::ReplayClipNotify,        true },
        { ConfigKeys::ReplayResolution,        "1920x1080" },
        { ConfigKeys::ReplayFps,               30 },
        { ConfigKeys::ReplayBitrateMbps,       14 },
        // Rolling replay-buffer length, in seconds. UI dropdown exposes the
        // allowed set {30,60,180,300,600,900} (30s/1m/3m/5m/10m/15m); default 5 min.
        { ConfigKeys::ReplayLengthSeconds,     300 },
        // Internal segment granularity for the disk-backed ring (not in the UI).
        // Ring size = ceil(length_seconds / segment_seconds).
        { ConfigKeys::ReplaySegmentSeconds,    5 },
        // Always-on recording: auto-arm the replay buffer whenever a game is
        // foreground (per capture.mode). Settings → Replay is the master switch.
        { ConfigKeys::ReplayAuto,              true },
        { ConfigKeys::InputShareHoldMs,        2000 },
        { ConfigKeys::AudioEnabled,            false },
        { ConfigKeys::StorageScreenshotsRoot,  "" },
        { ConfigKeys::StorageClipsRoot,        "" },
        { ConfigKeys::StartupEnabled,          false },
        { ConfigKeys::StartupMinimized,        false },
        { ConfigKeys::SoundsEnabled,           true },
        { ConfigKeys::SoundsVolume,            80 },
        { ConfigKeys::TrayCloseToTray,         true },
        { ConfigKeys::TrayMinimizeToTray,      false },
        { ConfigKeys::NotificationsEnabled,    true },
        { ConfigKeys::ThemeActiveSkin,         "dark" },  // dark | light | high_contrast
        { ConfigKeys::ThemeOverlayScrimStrength, 100 },   // percent, 25-150
    };
}

bool ConfigManager::load()
{
    m_overrides = {};
    QFile file(m_filePath);
    if (!file.exists())
        return true; // first run → defaults
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Config: cannot read" << m_filePath;
        return false;
    }
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Config: parse error in" << m_filePath << err.errorString();
        return false;
    }
    // Persisted JSON stores overrides only. Older releases wrote every default
    // into the file; values equal to today's defaults are safely canonicalized
    // away while unknown keys remain untouched for forward compatibility.
    const QJsonObject loaded = doc.object();
    const QJsonObject builtIns = defaults();
    for (auto it = loaded.begin(); it != loaded.end(); ++it) {
        if (builtIns.contains(it.key()) && builtIns.value(it.key()) == it.value())
            continue;
        m_overrides.insert(it.key(), it.value());
    }
    return true;
}

bool ConfigManager::save() const
{
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Config: cannot write" << m_filePath;
        return false;
    }
    file.write(QJsonDocument(m_overrides).toJson(QJsonDocument::Indented));
    return file.commit();
}

QVariant ConfigManager::value(const QString& key, const QVariant& fallback) const
{
    const QJsonValue overrideValue = m_overrides.value(key);
    if (!overrideValue.isUndefined())
        return overrideValue.toVariant();
    return defaultValue(key, fallback);
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
    if (this->value(key) == value)
        return;

    const QJsonValue jsonValue = QJsonValue::fromVariant(value);
    const QJsonObject builtIns = defaults();
    if (builtIns.contains(key) && builtIns.value(key) == jsonValue)
        m_overrides.remove(key);
    else
        m_overrides.insert(key, jsonValue);
    emit valueChanged(key, this->value(key));
}

QVariant ConfigManager::defaultValue(const QString& key, const QVariant& fallback) const
{
    const QJsonValue v = defaults().value(key);
    return v.isUndefined() ? fallback : v.toVariant();
}

bool ConfigManager::isDefault(const QString& key) const
{
    return !m_overrides.contains(key);
}

bool ConfigManager::resetValue(const QString& key)
{
    if (!m_overrides.contains(key))
        return false;
    m_overrides.remove(key);
    emit valueChanged(key, defaultValue(key));
    return true;
}

bool ConfigManager::resetGroup(const QString& prefix)
{
    const QString normalized = prefix.endsWith(QLatin1Char('.'))
        ? prefix : prefix + QLatin1Char('.');
    QStringList removed;
    for (auto it = m_overrides.constBegin(); it != m_overrides.constEnd(); ++it) {
        if (it.key().startsWith(normalized))
            removed.append(it.key());
    }
    for (const QString& key : removed) {
        m_overrides.remove(key);
        emit valueChanged(key, defaultValue(key));
    }
    if (!removed.isEmpty())
        emit groupReset(prefix);
    return !removed.isEmpty();
}

bool ConfigManager::resetAll()
{
    // internal.* entries are safety metadata rather than user settings. In
    // particular, capture-root history must survive Restore all so old media
    // remains discoverable without moving or deleting it.
    QStringList removed;
    for (auto it = m_overrides.constBegin(); it != m_overrides.constEnd(); ++it) {
        if (!it.key().startsWith(QStringLiteral("internal.")))
            removed.append(it.key());
    }
    if (removed.isEmpty())
        return false;
    for (const QString& key : removed) {
        m_overrides.remove(key);
        emit valueChanged(key, defaultValue(key));
    }
    emit groupReset(QString());
    return true;
}
