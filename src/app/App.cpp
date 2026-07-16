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
#include "notify/NotificationCenter.h"
#include "overlay/OverlayManager.h"
#include "sound/SoundEngine.h"
#include "storage/CaptureDatabase.h"
#include "storage/CaptureScanner.h"
#include "tray/TrayIcon.h"
#include "ui/AppController.h"
#include "ui/GalleryModel.h"
#include "Brand.h"

#include <QDateTime>
#include <QFile>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

App::App(QObject* parent)
    : QObject(parent)
{
}

App::~App() = default;

bool App::init()
{
    Paths::ensureDirectories();
    Logger::install(Paths::logsDir());

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

    const QString databasePath = Paths::dataDir() + QStringLiteral("/gamehq.db");
    for (const QString& legacyName : {QStringLiteral("saveplay.db"),
                                      QStringLiteral("playhq.db")}) {
        const QString legacyPath = Paths::dataDir() + QLatin1Char('/') + legacyName;
        if (!QFile::exists(databasePath) && QFile::exists(legacyPath))
            QFile::rename(legacyPath, databasePath);
    }
    m_db = std::make_unique<CaptureDatabase>(databasePath);
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

    // Screenshot capture (0.4): GDI grab of the foreground game → DB/gallery.
    m_screenshots = std::make_unique<ScreenshotService>(m_config.get(),
                                                        m_locations.get());
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
    // (replay.auto, always-on). Ctrl+Shift+R toggles that master switch; Share-hold
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
    connect(m_input.get(), &InputEngine::bufferToggleRequested,
            m_framePump.get(), &FramePumpService::toggle);
    connect(m_overlay.get(), &OverlayManager::visibleChanged, this, [this] {
        m_input->setOverlayVisible(m_overlay->isVisible());
    });

    m_engine.rootContext()->setContextProperty(QStringLiteral("app"), m_controller.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("overlayGallery"), m_overlayGallery.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("overlay"), m_overlay.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("sounds"), m_sounds.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("input"), m_input.get());
    m_engine.rootContext()->setContextProperty(QStringLiteral("notifications"), m_notify.get());
    m_engine.loadFromModule("GameHQ", "Main");
    if (m_engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root — aborting startup";
        return false;
    }

    m_input->start();   // seed default bindings + begin listening for the pad

    // First scan after the window is up so startup feels instant.
    QTimer::singleShot(0, m_controller.get(), &AppController::rescan);
    return true;
}

void App::showWindow()
{
    if (m_engine.rootObjects().isEmpty())
        return;
    if (auto* window = qobject_cast<QQuickWindow*>(m_engine.rootObjects().first())) {
        window->show();
        window->raise();
        window->requestActivate();
    }
}
