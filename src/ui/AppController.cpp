#include "ui/AppController.h"
#include "app/StartupManager.h"
#include "config/CaptureLocations.h"
#include "config/ConfigManager.h"
#include "config/Paths.h"
#include "storage/CaptureDatabase.h"
#include "storage/CaptureScanner.h"
#include "ui/CaptureLibraryService.h"
#include "ui/CurrentGameService.h"
#include "ui/GalleryModel.h"
#include "ui/ShellActions.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QGuiApplication>
#include <QHash>
#include <QVariantMap>

namespace
{
bool parseCaptureKind(const QString& value, CaptureLocations::Kind& kind)
{
    if (value == QStringLiteral("screenshots")) {
        kind = CaptureLocations::Kind::Screenshots;
        return true;
    }
    if (value == QStringLiteral("clips")) {
        kind = CaptureLocations::Kind::Clips;
        return true;
    }
    return false;
}

// replay.clip_sound / replay.clip_notify only affect the "saved" toast, not the
// recording pipeline — rearming the buffer for them would discard its ring for
// no benefit. Every other replay.* key (and audio.enabled) is an actual
// recording parameter and must re-arm a running buffer to take effect.
bool isReplayBufferParamKey(const QString& key)
{
    if (key == QStringLiteral("replay.clip_sound") || key == QStringLiteral("replay.clip_notify"))
        return false;
    return key.startsWith(QStringLiteral("replay.")) || key == QStringLiteral("audio.enabled");
}
}

AppController::AppController(CaptureDatabase* db, CaptureScanner* scanner,
                             GalleryModel* gallery, GalleryModel* overlayGallery,
                             ConfigManager* config, CaptureLocations* locations,
                             StartupManager* startup,
                             QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_scanner(scanner)
    , m_gallery(gallery)
    , m_overlayGallery(overlayGallery)
    , m_config(config)
    , m_locations(locations)
    , m_startup(startup)
    , m_captureLibrary(std::make_unique<CaptureLibraryService>(db, gallery, overlayGallery))
    , m_currentGame(std::make_unique<CurrentGameService>(db))
{
    connect(m_config, &ConfigManager::valueChanged,
            this, &AppController::configChanged);
    connect(m_config, &ConfigManager::groupReset,
            this, &AppController::configGroupReset);
    connect(m_locations, &CaptureLocations::locationsChanged,
            this, &AppController::captureLocationsChanged);
}

AppController::~AppController() = default;

QStringList AppController::watchedFolders() const
{
    return m_db->watchedFolders();
}

QString AppController::capturesRoot() const
{
    return QDir::toNativeSeparators(Paths::capturesRoot());
}

QString AppController::screenshotsRoot() const
{
    return QDir::toNativeSeparators(m_locations->screenshotsBaseRoot());
}

QString AppController::clipsRoot() const
{
    return QDir::toNativeSeparators(m_locations->clipsBaseRoot());
}

QStringList AppController::managedRoots() const
{
    QStringList roots;
    for (const QString& root : m_locations->managedRoots())
        roots.append(QDir::toNativeSeparators(root));
    return roots;
}

bool AppController::startMinimized() const
{
    return m_config->value(QStringLiteral("startup.minimized"), false).toBool();
}

bool AppController::portableMode() const
{
    return Paths::isPortable();
}

QString AppController::dataRoot() const
{
    return QDir::toNativeSeparators(Paths::dataDir());
}

QString AppController::logsRoot() const
{
    return QDir::toNativeSeparators(Paths::logsDir());
}

void AppController::openDataFolder()
{
    ShellActions::openFile(Paths::dataDir());
}

void AppController::openLogsFolder()
{
    ShellActions::openFile(Paths::logsDir());
}

void AppController::quitApplication()
{
    QCoreApplication::quit();
}

void AppController::copyDiagnosticSummary() const
{
    const QString summary = QStringLiteral(
        "GameHQ %1 (%2)\n"
        "Data folder: %3\n"
        "Logs folder: %4\n"
        "Screenshots folder: %5\n"
        "Clips folder: %6")
        .arg(version(), portableMode() ? QStringLiteral("portable") : QStringLiteral("installed"),
             dataRoot(), logsRoot(), screenshotsRoot(), clipsRoot());
    QGuiApplication::clipboard()->setText(summary);
}

QString AppController::setCaptureRoot(const QString& kindValue, const QUrl& folderUrl)
{
    CaptureLocations::Kind kind;
    if (!parseCaptureKind(kindValue, kind))
        return QStringLiteral("The capture type is invalid.");
    if (!folderUrl.isLocalFile())
        return QStringLiteral("Choose a local folder.");

    QString error;
    if (!m_locations->setBaseRoot(kind, folderUrl.toLocalFile(), &error))
        return error;
    rescan();
    return {};
}

QString AppController::resetCaptureRoot(const QString& kindValue)
{
    CaptureLocations::Kind kind;
    if (!parseCaptureKind(kindValue, kind))
        return QStringLiteral("The capture type is invalid.");

    QString error;
    if (!m_locations->resetBaseRoot(kind, &error))
        return error;
    rescan();
    return {};
}

void AppController::removeWatchedFolder(const QString& path)
{
    m_db->removeWatchedFolder(path);
    emit foldersChanged();
}

QVariant AppController::config(const QString& key, const QVariant& fallback) const
{
    return m_config->value(key, fallback);
}

void AppController::setConfig(const QString& key, const QVariant& value)
{
    if (key == QStringLiteral("startup.enabled")
        && !m_startup->setEnabled(value.toBool())) {
        qWarning() << "Startup: setting change was rejected";
        emit configChanged(key, m_config->value(key, false));
        return;
    }
    CaptureLocations::Kind locationKind;
    if (key == QStringLiteral("storage.screenshots_root")
        || key == QStringLiteral("storage.clips_root")) {
        const QString kindValue = key == QStringLiteral("storage.screenshots_root")
            ? QStringLiteral("screenshots") : QStringLiteral("clips");
        parseCaptureKind(kindValue, locationKind);
        QString error;
        if (!m_locations->setBaseRoot(locationKind, value.toString(), &error))
            qWarning() << "Capture location:" << error;
        else
            rescan();
        return;
    }
    m_config->setValue(key, value);
    m_config->save();
    // A recording buffer armed with the old fps/resolution/length must pick
    // up the change — App connects this to FramePumpService::restartBuffer.
    if (isReplayBufferParamKey(key))
        emit replaySettingsChanged();
}

QVariant AppController::configDefault(const QString& key, const QVariant& fallback) const
{
    return m_config->defaultValue(key, fallback);
}

bool AppController::configIsDefault(const QString& key) const
{
    return m_config->isDefault(key);
}

void AppController::resetConfig(const QString& key)
{
    if (key == QStringLiteral("startup.enabled") && !m_startup->setEnabled(false)) {
        emit configChanged(key, m_config->value(key, false));
        return;
    }
    if (key == QStringLiteral("storage.screenshots_root")) {
        resetCaptureRoot(QStringLiteral("screenshots"));
        return;
    }
    if (key == QStringLiteral("storage.clips_root")) {
        resetCaptureRoot(QStringLiteral("clips"));
        return;
    }
    if (!m_config->resetValue(key))
        return;
    m_config->save();
    if (isReplayBufferParamKey(key))
        emit replaySettingsChanged();
}

void AppController::resetConfigGroup(const QString& prefix)
{
    if (prefix == QStringLiteral("startup") || prefix.isEmpty())
        m_startup->setEnabled(false);
    if (prefix == QStringLiteral("storage") || prefix.isEmpty())
        m_locations->preserveCurrentRoots();
    if (!m_config->resetGroup(prefix))
        return;
    m_config->save();
    if (prefix == QStringLiteral("replay") || prefix == QStringLiteral("audio")
        || prefix.isEmpty())
        emit replaySettingsChanged();
    if (prefix == QStringLiteral("storage") || prefix.isEmpty())
        rescan();
}

void AppController::resetAllConfig()
{
    m_startup->setEnabled(false);
    m_locations->preserveCurrentRoots();
    if (!m_config->resetAll())
        return;
    m_config->save();
    emit replaySettingsChanged();
    rescan();
}

void AppController::resetCategory(const QString& category)
{
    // Each entry lists every config prefix that page actually reads/writes.
    // Input bindings have their own database-backed restore path. Library has
    // no config group of its own (watched folders are DB rows). The screenshot/clip
    // folder pickers live on the Capture page even though the clip root also
    // feeds Replay, so both single-key resets (not the shared "storage."
    // group, which would silently affect both) belong to "Capture" here.
    static const QHash<QString, QStringList> kCategoryGroups = {
        { QStringLiteral("General"),               { QStringLiteral("startup"), QStringLiteral("tray") } },
        { QStringLiteral("Capture"),                { QStringLiteral("capture") } },
        { QStringLiteral("Replay"),                 { QStringLiteral("replay"), QStringLiteral("audio") } },
        { QStringLiteral("Notifications & Sound"),  { QStringLiteral("sounds"), QStringLiteral("notifications") } },
    };
    for (const QString& prefix : kCategoryGroups.value(category))
        resetConfigGroup(prefix);
    if (category == QStringLiteral("Capture")) {
        resetConfig(QStringLiteral("storage.screenshots_root"));
        resetConfig(QStringLiteral("storage.clips_root"));
    }
}

QString AppController::version() const
{
    return QStringLiteral(GAMEHQ_VERSION);
}

int AppController::currentGameId() const
{
    return m_currentGame->currentGameId();
}

bool AppController::currentGameAvailable() const
{
    return m_currentGame->currentGameAvailable();
}

int AppController::overlayGameId() const
{
    return m_currentGame->currentGameId();
}

QVariantList AppController::games() const
{
    QVariantList out;
    const auto entries = m_db->listGames();
    for (const GameEntry& g : entries) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), g.id);
        item.insert(QStringLiteral("name"), g.name);
        item.insert(QStringLiteral("iconPath"), g.iconPath);
        out.append(item);
    }
    return out;
}

void AppController::setCategory(const QString& category)
{
    m_category = category;
    m_gameId = -1;
    m_gallery->setFilter(m_category, m_gameId);
    emit filterChanged();
}

void AppController::setGame(int gameId)
{
    m_category = QStringLiteral("all");
    m_gameId = gameId;
    m_gallery->setFilter(m_category, m_gameId);
    emit filterChanged();
}

void AppController::setGameCategory(const QString& category, int gameId)
{
    m_category = category;
    m_gameId = gameId;
    m_gallery->setFilter(m_category, m_gameId);
    emit filterChanged();
}

void AppController::syncOverlayToForegroundGame()
{
    const bool stateChanged = m_currentGame->syncToForegroundGame();
    if (stateChanged) {
        emit currentGameChanged();
        emit overlayGameChanged();
    }
    if (m_currentGame->lastUpdateChangedGameMetadata())
        emit gamesChanged();
    if (m_overlayGallery) {
        const int gameId = m_currentGame->currentGameAvailable() ? m_currentGame->currentGameId() : -1;
        m_overlayGallery->setFilter(QStringLiteral("all"), gameId);
    }
}

void AppController::updateForegroundGame(const QString& gameName, const QString& executablePath)
{
    const bool stateChanged = m_currentGame->update(gameName, executablePath);
    if (stateChanged) {
        emit currentGameChanged();
        emit overlayGameChanged();
    }

    if (m_currentGame->lastUpdateChangedGameMetadata())
        emit gamesChanged();
}

void AppController::updateReplayBufferState(bool active, const QString& gameName)
{
    if (m_replayBufferActive == active && m_replayBufferGame == gameName)
        return;
    m_replayBufferActive = active;
    m_replayBufferGame = gameName;
    emit replayBufferStateChanged();
}

void AppController::rescan()
{
    m_lastScanAdded = m_scanner->scanAll();
    m_gallery->refresh();
    emit gamesChanged();
    emit lastScanChanged();
}

void AppController::toggleFavorite(int row)
{
    m_gallery->toggleFavorite(row);
}

void AppController::deleteCapture(int row) { deleteCaptureFrom(m_gallery, row); }
void AppController::openCapture(int row) { openCaptureFrom(m_gallery, row); }
void AppController::showInFolder(int row) { showInFolderFrom(m_gallery, row); }

void AppController::deleteCaptures(const QVariantList& rows)
{
    if (!m_captureLibrary->deleteCaptures(m_gallery, rows))
        return;
    emit gamesChanged();
}

void AppController::deleteCaptureFrom(GalleryModel* model, int row)
{
    if (m_captureLibrary->deleteCapture(model, row))
        emit gamesChanged();
}

void AppController::openCaptureFrom(GalleryModel* model, int row)
{
    m_captureLibrary->openCapture(model, row);
}

void AppController::showInFolderFrom(GalleryModel* model, int row)
{
    m_captureLibrary->showInFolder(model, row);
}

void AppController::commitCapture(const QString& filePath, const QString& type,
                                  const QString& gameName, const QString& executablePath)
{
    m_captureLibrary->commitCapture(filePath, type, gameName, executablePath);
    const bool stateChanged = m_currentGame->update(gameName, executablePath);
    if (stateChanged) {
        emit currentGameChanged();
        emit overlayGameChanged();
    }
    emit gamesChanged();
}

void AppController::commitClip(const QString& filePath, const QString& gameName,
                              const QString& thumbnailPath, const QString& executablePath)
{
    m_captureLibrary->commitClip(filePath, gameName, thumbnailPath, executablePath);
    const bool stateChanged = m_currentGame->update(gameName, executablePath);
    if (stateChanged) {
        emit currentGameChanged();
        emit overlayGameChanged();
    }
    emit gamesChanged();
}

void AppController::rememberGameExecutable(const QString& gameName, const QString& executablePath)
{
    if (!m_db->rememberGameExecutable(gameName, executablePath))
        return;
    emit gamesChanged();
}

void AppController::addWatchedFolder(const QUrl& folderUrl)
{
    const QString path = folderUrl.toLocalFile();
    if (path.isEmpty())
        return;
    m_db->addWatchedFolder(path, QStringLiteral("Imported"));
    emit foldersChanged();
    rescan();
}
