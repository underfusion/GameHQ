#include "app/App.h"
#include "app/StartupManager.h"
#include "config/ConfigKeys.h"
#include "capture/FramePumpService.h"
#include "capture/ScreenshotService.h"
#include "config/CaptureLocations.h"
#include "config/Paths.h"
#include "config/ConfigManager.h"
#include "diagnostics/Logger.h"
#include "input/HotkeyManager.h"
#include "input/InputEngine.h"
#include "integration/IntegrationService.h"
#include "games/GameDetector.h"
#include "core/UpdateMaintenance.h"
#include "notify/NotificationCenter.h"
#include "overlay/OverlayManager.h"
#include "sound/SoundEngine.h"
#include "storage/CaptureDatabase.h"
#include "storage/CaptureScanner.h"
#include "tray/TrayIcon.h"
#include "ui/AppController.h"
#include "ui/GalleryModel.h"
#include "updates/UpdateService.h"
#include "updates/UpdateInstaller.h"
#include "Brand.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

App::App(QObject* parent)
    : QObject(parent)
{
}

App::~App()
{
    GameDetector::setExternalContext(nullptr);
}

void App::setPostUpdateValidation(bool enabled)
{
    m_postUpdateValidation = enabled;
}

void App::recordPostUpdateSuccess(const QString& version)
{
    if (!m_postUpdateValidation || !m_controller)
        return;
    m_controller->setConfig(QString(ConfigKeys::InternalUpdatesPendingPostUpdateVersion), version);
}

bool App::init()
{
    Paths::ensureDirectories();
    Logger::install(Paths::logsDir());

    m_integration = std::make_unique<IntegrationService>(QStringLiteral(GAMEHQ_VERSION));
    connect(m_integration.get(), &IntegrationService::activateRequested,
            this, &App::showWindow);
    connect(m_integration.get(), &IntegrationService::openGalleryRequested,
            this, &App::openGallery);
    QString integrationError;
    if (!m_integration->start(integrationError))
        qWarning() << "Integration server unavailable:" << integrationError;
    GameDetector::setExternalContext(m_integration->externalContext());

    qInfo() << Brand::Name << GAMEHQ_VERSION
            << "starting, portable:" << Paths::isPortable()
            << "data:" << Paths::dataDir();

    m_config = std::make_unique<ConfigManager>(Paths::dataDir() + QStringLiteral("/config.json"));
    m_config->load();
    m_locations = std::make_unique<CaptureLocations>(m_config.get());
    m_startup = std::make_unique<StartupManager>();
    const bool startupEnabled = m_config->value(ConfigKeys::StartupEnabled, false).toBool();
    if (!m_startup->setEnabled(startupEnabled) && startupEnabled) {
        m_config->setValue(ConfigKeys::StartupEnabled, false);
        m_config->save();
    }

    m_db = std::make_unique<CaptureDatabase>(Paths::databasePath());
    if (!m_db->open()) {
        qCritical() << "Failed to open database — aborting startup";
        return false;
    }

    m_scanner = std::make_unique<CaptureScanner>(m_db.get(), m_locations.get(),
                                                 Paths::thumbnailsDir());
    m_gallery = std::make_unique<GalleryModel>(m_db.get());
    m_overlayGallery = std::make_unique<GalleryModel>(m_db.get());
    m_controller = std::make_unique<AppController>(m_db.get(), m_scanner.get(),
                                                   m_gallery.get(), m_overlayGallery.get(),
                                                   m_config.get(), m_locations.get(), m_startup.get());

    // Ordering contract for everything below: the service lambdas capture `this`
    // and dereference members that are constructed further down (m_sounds at the
    // SoundEngine block, m_notify at the NotificationCenter block). That is safe
    // only because nothing here emits before init() returns — the first capture
    // needs the pad/hotkeys, which m_input->start() arms last. Keep it that way:
    // if a service ever fires a signal from its own constructor, construct
    // m_sounds/m_notify before the connect that uses them.

    // Screenshot capture (0.4): GDI grab of the foreground game → DB/gallery.
    m_screenshots = std::make_unique<ScreenshotService>(m_config.get(),
                                                        m_locations.get());
    // Let the QML bridge save clip frames (Share on a focused clip) through the
    // same screenshot pipeline — same folder, format, sound and toast.
    m_controller->setScreenshotService(m_screenshots.get());
    // Shutter plays the instant the pixels are grabbed (before the async encode),
    // so the feedback is immediate even though the PNG lands a moment later.
    connect(m_screenshots.get(), &ScreenshotService::grabbed, this,
            [this] {
                if (m_config->value(ConfigKeys::CaptureScreenshotSound, true).toBool())
                    m_sounds->play(QStringLiteral("screenshot"));
            });
    connect(m_screenshots.get(), &ScreenshotService::captured, this,
            [this](const QString& path, const QString& game, const QString& exePath) {
                m_controller->commitCapture(path, QStringLiteral("screenshot"), game, exePath);
                if (m_config->value(ConfigKeys::NotificationsEnabled, true).toBool()
                    && m_config->value(ConfigKeys::CaptureScreenshotNotify, true).toBool()) {
                    const QString when = QDateTime::currentDateTime().toString(QStringLiteral("d MMM yyyy, HH:mm"));
                    m_notify->post(tr("Screenshot saved"), game, path,
                                   QStringLiteral("success"), when, false);
                }
            });
    connect(m_screenshots.get(), &ScreenshotService::skipped, this,
            [this](const QString& why) {
                qInfo() << "Screenshot: skipped —" << why;
                m_sounds->play(QStringLiteral("error"));
            });
    connect(m_screenshots.get(), &ScreenshotService::failed, this,
            [this](const QString& why) {
                qWarning() << "Screenshot: failed —" << why;
                m_sounds->play(QStringLiteral("error"));
            });

    // WGC replay buffer (0.5): records continuously while a game is foreground
    // (replay.auto, always-on; Settings → Replay is the master switch). Share-hold
    // (or Ctrl+Shift+E) saves the last N seconds as one clip (below).
    m_framePump = std::make_unique<FramePumpService>(m_config.get(), m_locations.get());
    connect(m_framePump.get(), &FramePumpService::failed, this,
            [this](const QString& why) { qWarning() << "FramePump: failed —" << why; });
    // The ring freezes immediately, but do not say "saved" until the final MP4
    // exists and the gallery commit path runs. Otherwise a remux failure sounds
    // successful while no thumbnail appears.
    connect(m_framePump.get(), &FramePumpService::clipSaving, this,
            [](const QString&, const QString&, const QString&) {
            });
    // Remux finished: file + thumbnail ready → add it to the gallery and notify.
    connect(m_framePump.get(), &FramePumpService::clipSaved, this,
            [this](const QString& path, const QString& game, const QString& thumb,
                   const QString& exePath) {
                m_controller->commitClip(path, game, thumb, exePath);
                if (m_config->value(ConfigKeys::ReplayClipSound, true).toBool())
                    m_sounds->play(QStringLiteral("replay_saved"));
                if (m_config->value(ConfigKeys::NotificationsEnabled, true).toBool()
                    && m_config->value(ConfigKeys::ReplayClipNotify, true).toBool()) {
                    const QString when = QDateTime::currentDateTime().toString(QStringLiteral("d MMM yyyy, HH:mm"));
                    m_notify->post(tr("Replay saved"), game, thumb,
                                   QStringLiteral("success"), when, true);
                }
            });
    connect(m_framePump.get(), &FramePumpService::clipFailed, this,
            [this](const QString& game, const QString& reason) {
                m_sounds->play(QStringLiteral("error"));
                if (m_config->value(ConfigKeys::NotificationsEnabled, true).toBool()) {
                    const QString when = QDateTime::currentDateTime().toString(QStringLiteral("d MMM yyyy, HH:mm"));
                    m_notify->post(tr("Replay failed"), reason.isEmpty() ? game : reason,
                                   QString(), QStringLiteral("error"), when, false);
                }
            });
    connect(m_framePump.get(), &FramePumpService::foregroundGameDetected,
            m_controller.get(), &AppController::updateForegroundGame);
    connect(m_framePump.get(), &FramePumpService::recordingStateChanged,
            m_controller.get(), &AppController::updateReplayBufferState);
    // Settings changed a replay.* key (fps/resolution/length): re-arm a
    // running buffer so it records with the new parameters immediately.
    connect(m_controller.get(), &AppController::replaySettingsChanged,
            m_framePump.get(), &FramePumpService::restartBuffer);

    m_tray = std::make_unique<TrayIcon>();
    connect(m_tray.get(), &TrayIcon::openGalleryRequested, this, &App::showWindow);
    connect(m_tray.get(), &TrayIcon::rescanRequested,
            m_controller.get(), &AppController::rescan);
    connect(m_tray.get(), &TrayIcon::screenshotRequested,
            m_screenshots.get(), &ScreenshotService::capture);
    connect(m_tray.get(), &TrayIcon::quitRequested,
            qApp, &QCoreApplication::quit, Qt::QueuedConnection);

    m_overlay = std::make_unique<OverlayManager>(&m_engine);
    m_notify = std::make_unique<NotificationCenter>(&m_engine);
    m_updates = std::make_unique<UpdateService>(QStringLiteral("underfusion"), QStringLiteral("GameHQ"),
                                                 QStringLiteral(GAMEHQ_VERSION),
                                                 Paths::packageRoot() + QStringLiteral("/.update/downloads"),
                                                 nullptr);
    // Update-check policy (docs/updater.md "Discovery"): prime the cached ETag
    // and skip flag from config, persist whatever the service later reports,
    // and gate automatic checks to at most once every 24h. Manual checkNow()
    // from QML always bypasses this cache.
    m_updates->primeCachedEtag(m_config->value(ConfigKeys::InternalUpdatesEtag, QString()).toString());
    m_updates->primeSkippedVersion(m_config->value(ConfigKeys::UpdatesSkippedVersion, QString()).toString());
    connect(m_updates.get(), &UpdateService::etagUpdated, this, [this](const QString& etag) {
        m_config->setValue(ConfigKeys::InternalUpdatesEtag, etag);
        m_config->save();
    });
    connect(m_updates.get(), &UpdateService::versionSkipped, this, [this](const QString& version) {
        m_config->setValue(ConfigKeys::UpdatesSkippedVersion, version);
        m_config->save();
    });
    connect(m_updates.get(), &UpdateService::lastCheckedChanged, this, [this] {
        m_config->setValue(ConfigKeys::InternalUpdatesLastCheckUtc,
                            m_updates->lastChecked().toUTC().toString(Qt::ISODate));
        m_config->save();
    });
    m_updatePreparationTimer = new QTimer(this);
    m_updatePreparationTimer->setSingleShot(true);
    m_updatePreparationTimer->setInterval(30000);
    auto finishUpdatePreparation = [this] {
        if (!m_updateScreenshotsReady || !m_updateReplayReady)
            return;
        m_updatePreparationTimer->stop();
        m_config->save();
        qInfo() << "Update preparation: capture services are quiescent and config is flushed";
        m_updates->markQuiescent();
    };
    connect(m_screenshots.get(), &ScreenshotService::updateReady, this, [this, finishUpdatePreparation] {
        m_updateScreenshotsReady = true;
        finishUpdatePreparation();
    });
    connect(m_framePump.get(), &FramePumpService::updateWaitingForExport, this, [] {
        qInfo() << "Update preparation: waiting for the current replay export to finish";
    });
    connect(m_framePump.get(), &FramePumpService::updateReady, this, [this, finishUpdatePreparation] {
        m_updateReplayReady = true;
        finishUpdatePreparation();
    });
    connect(m_updates.get(), &UpdateService::prepareForUpdateRequested, this, [this] {
        m_updateScreenshotsReady = false;
        m_updateReplayReady = false;
        m_updatePreparationTimer->start();
        m_screenshots->prepareForUpdate();
        m_framePump->prepareForUpdate();
    });
    connect(m_updates.get(), &UpdateService::installApproved, this,
            [this](const QString &version, const QString &packagePath, const QByteArray &sha256) {
        QString transactionPath;
        QString error;
        if (!UpdateInstaller::prepareTransaction(Paths::packageRoot(), Paths::dataDir(),
                                                  packagePath, version, sha256,
                                                  transactionPath, error)) {
            m_screenshots->cancelUpdatePreparation();
            m_framePump->cancelUpdatePreparation();
            m_updates->cancelPreparation(error);
            qWarning() << "Update installer handoff failed:" << error;
            return;
        }
        std::string maintenanceError;
        if (!maintenance::begin(Paths::packageRoot().toStdWString(), maintenanceError)) {
            error = QString::fromStdString(maintenanceError);
            m_screenshots->cancelUpdatePreparation();
            m_framePump->cancelUpdatePreparation();
            m_updates->cancelPreparation(error);
            qWarning() << "Update maintenance handoff failed:" << error;
            return;
        }
        m_integration->broadcastMaintenance(300);
        if (!UpdateInstaller::launchPrepared(Paths::packageRoot(), transactionPath, error)) {
            maintenance::finish(Paths::packageRoot().toStdWString());
            m_integration->cancelMaintenance();
            m_screenshots->cancelUpdatePreparation();
            m_framePump->cancelUpdatePreparation();
            m_updates->cancelPreparation(error);
            qWarning() << "Update installer handoff failed:" << error;
            return;
        }
        m_updates->markInstalling();
        qInfo() << "Updater helper launched for" << version << "; exiting application";
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    });
    connect(m_updatePreparationTimer, &QTimer::timeout, this, [this] {
        m_screenshots->cancelUpdatePreparation();
        m_framePump->cancelUpdatePreparation();
        m_updates->cancelPreparation(QStringLiteral(
            "The update was cancelled because capture work did not finish safely in time."));
        qWarning() << "Update preparation timed out; update cancelled without stopping capture work";
    });
    auto maybeAutoCheckForUpdate = [this] {
        if (!m_config->value(ConfigKeys::UpdatesCheckAutomatically, true).toBool())
            return;
        const QDateTime last = QDateTime::fromString(
            m_config->value(ConfigKeys::InternalUpdatesLastCheckUtc, QString()).toString(), Qt::ISODate);
        if (last.isValid() && last.secsTo(QDateTime::currentDateTimeUtc()) < 24 * 3600)
            return;
        m_updates->checkNow();
    };
    QTimer::singleShot(20000, m_updates.get(), maybeAutoCheckForUpdate); // 15-30s after startup
    m_updateCheckTimer = new QTimer(this);
    m_updateCheckTimer->setInterval(60 * 60 * 1000); // hourly wake; the 24h gate above does the real throttling
    connect(m_updateCheckTimer, &QTimer::timeout, this, maybeAutoCheckForUpdate);
    m_updateCheckTimer->start();

    m_hotkeys = std::make_unique<HotkeyManager>();
    connect(m_overlay.get(), &OverlayManager::aboutToShow,
            m_controller.get(), &AppController::syncOverlayToForegroundGame);

    m_sounds = std::make_unique<SoundEngine>(m_config.get());
    connect(m_overlay.get(), &OverlayManager::visibleChanged, this, [this] {
        m_sounds->play(m_overlay->isVisible() ? QStringLiteral("overlay_open")
                                              : QStringLiteral("overlay_close"));
    });

    // Controller input (0.3): DualSense Share tap/hold + PS, keyboard hotkey stays.
    m_input = std::make_unique<InputEngine>(m_config.get(), m_db.get(), m_hotkeys.get());
    connect(m_input.get(), &InputEngine::overlayToggleRequested,
            m_overlay.get(), &OverlayManager::toggle);
    // overlayHideRequested (Circle) is now consumed in OverlayWindow.qml: it
    // pops the action menu / sidebar focus first and only closes the overlay
    // at the root level, calling `overlay.hide()` itself in that case.
    connect(m_input.get(), &InputEngine::screenshotRequested,
            m_screenshots.get(), &ScreenshotService::capture);
    connect(m_input.get(), &InputEngine::replayRequested,
            m_framePump.get(), &FramePumpService::saveReplay);
    connect(m_overlay.get(), &OverlayManager::visibleChanged, this, [this] {
        m_input->setOverlayVisible(m_overlay->isVisible());
    });

    m_engine.rootContext()->setContextProperty(QStringLiteral("app"), m_controller.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("overlayGallery"), m_overlayGallery.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("overlay"), m_overlay.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("sounds"), m_sounds.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("input"), m_input.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("updates"), m_updates.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("notifications"), m_notify.get());
    m_engine.loadFromModule("GameHQ", "Main");
    if (m_engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root — aborting startup";
        return false;
    }
    if (m_pendingOpenGallery)
        openGallery();
    else if (m_pendingActivation)
        showWindow();

    if (m_postUpdateValidation) {
        // A newly installed build must prove that its database, QML and services
        // stay alive before capture hooks are armed. The helper watches for the
        // token written by main.cpp after the event-loop health window.
        m_screenshots->prepareForUpdate();
        m_framePump->prepareForUpdate();
    } else {
        m_input->start();   // seed default bindings + begin listening for the pad
        m_inputStarted = true;
    }

    // First scan after the window is up so startup feels instant. The HDR probe
    // rides along for the same reason — it activates an encoder MFT, which is
    // far too slow to run before the first frame.
    QTimer::singleShot(0, m_controller.get(), &AppController::rescan);
    QTimer::singleShot(0, m_controller.get(), &AppController::refreshHdrStatus);

    // HDR is a per-monitor toggle the user can flip at any time; re-probe when
    // the desktop topology changes so diagnostics never report a stale state.
    connect(qApp, &QGuiApplication::screenAdded, m_controller.get(), &AppController::refreshHdrStatus);
    connect(qApp, &QGuiApplication::screenRemoved, m_controller.get(), &AppController::refreshHdrStatus);
    connect(qApp, &QGuiApplication::primaryScreenChanged, m_controller.get(), &AppController::refreshHdrStatus);
    return true;
}

void App::completePostUpdateValidation()
{
    if (!m_postUpdateValidation)
        return;
    m_postUpdateValidation = false;
    m_screenshots->cancelUpdatePreparation();
    m_framePump->cancelUpdatePreparation();
    if (!m_inputStarted) {
        m_input->start();
        m_inputStarted = true;
    }
}

void App::showWindow()
{
    if (m_engine.rootObjects().isEmpty()) {
        m_pendingActivation = true;
        return;
    }
    m_pendingActivation = false;
    if (auto* window = qobject_cast<QQuickWindow*>(m_engine.rootObjects().first())) {
        window->show();
        window->raise();
        window->requestActivate();
    }
}

void App::openGallery()
{
    if (m_engine.rootObjects().isEmpty() || !m_controller) {
        m_pendingOpenGallery = true;
        return;
    }
    m_pendingOpenGallery = false;
    m_controller->setCategory(QStringLiteral("all"));
    QObject *root = m_engine.rootObjects().first();
    root->setProperty("settingsOpen", false);
    root->setProperty("helpOpen", false);
    showWindow();
}
