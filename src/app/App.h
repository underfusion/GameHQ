#pragma once
#include <QObject>
#include <QQmlApplicationEngine>
#include <memory>

class ConfigManager;
class CaptureLocations;
class StartupManager;
class CaptureDatabase;
class CaptureScanner;
class GalleryModel;
class AppController;
class TrayIcon;
class HotkeyManager;
class OverlayManager;
class SoundEngine;
class InputEngine;
class ScreenshotService;
class FramePumpService;
class NotificationCenter;

// Owns application lifecycle: paths → logging → config → database →
// scanner/model/controller → tray → QML UI.
class App : public QObject
{
    Q_OBJECT
public:
    explicit App(QObject* parent = nullptr);
    ~App() override;

    // Returns false on fatal init failure (already logged).
    bool init();

private:
    void showWindow();

    std::unique_ptr<ConfigManager> m_config;
    std::unique_ptr<CaptureLocations> m_locations;
    std::unique_ptr<StartupManager> m_startup;
    std::unique_ptr<CaptureDatabase> m_db;
    std::unique_ptr<CaptureScanner> m_scanner;
    std::unique_ptr<GalleryModel> m_gallery;
    // Separate instance for the overlay (milestone 0.6) so its own category/
    // game filter never clashes with the main window's.
    std::unique_ptr<GalleryModel> m_overlayGallery;
    std::unique_ptr<AppController> m_controller;
    std::unique_ptr<TrayIcon> m_tray;
    std::unique_ptr<HotkeyManager> m_hotkeys;
    std::unique_ptr<OverlayManager> m_overlay;
    std::unique_ptr<SoundEngine> m_sounds;
    std::unique_ptr<InputEngine> m_input;
    std::unique_ptr<ScreenshotService> m_screenshots;
    std::unique_ptr<FramePumpService> m_framePump;
    std::unique_ptr<NotificationCenter> m_notify;
    QQmlApplicationEngine m_engine;
};
