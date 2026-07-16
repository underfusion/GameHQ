qt_add_executable(GameHQ
    main.cpp
    app/App.cpp            app/App.h
    app/StartupManager.cpp app/StartupManager.h
    capture/CaptureUtil.h
    capture/ScreenshotService.cpp capture/ScreenshotService.h
    capture/FramePumpService.cpp capture/FramePumpService.h capture/wgc_shims.h
    capture/AudioCapture.cpp capture/AudioCapture.h capture/wasapi_shims.h
    capture/SegmentRecorder.cpp capture/SegmentRecorder.h
    capture/ReplayExporter.cpp capture/ReplayExporter.h
    config/Paths.cpp       config/Paths.h
    config/LegacyMigration.cpp config/LegacyMigration.h
    config/ConfigManager.cpp config/ConfigManager.h
    config/CaptureLocations.cpp config/CaptureLocations.h
    core/GameIdentity.cpp core/GameIdentity.h
    diagnostics/Logger.cpp diagnostics/Logger.h
    games/GameDetector.cpp games/GameDetector.h
    input/HotkeyManager.cpp input/HotkeyManager.h
    input/Gamepad.h
    input/ActionCatalog.cpp input/ActionCatalog.h
    input/BindingResolver.cpp input/BindingResolver.h
    input/BindingRuntime.cpp input/BindingRuntime.h
    input/BindingEditorModel.cpp input/BindingEditorModel.h
    input/ControlId.cpp input/ControlId.h
    input/DualSenseDevice.cpp input/DualSenseDevice.h
    input/XInputDevice.cpp input/XInputDevice.h
    input/WinMMDevice.cpp input/WinMMDevice.h
    input/MouseHookDevice.cpp input/MouseHookDevice.h
    input/InputEngine.cpp input/InputEngine.h
    notify/NotificationCenter.cpp notify/NotificationCenter.h
    overlay/OverlayManager.cpp overlay/OverlayManager.h
    sound/SoundEngine.cpp  sound/SoundEngine.h
    storage/CaptureDatabase.cpp storage/CaptureDatabase.h
    storage/CaptureQueries.cpp storage/CaptureQueries.h
    storage/CaptureScanner.cpp  storage/CaptureScanner.h
    storage/GameIconCache.cpp storage/GameIconCache.h
    storage/GameMetadataBackfill.cpp storage/GameMetadataBackfill.h
    storage/GameRowRepair.cpp storage/GameRowRepair.h
    storage/ThumbnailService.cpp storage/ThumbnailService.h
    tray/TrayIcon.cpp      tray/TrayIcon.h
    ui/AppController.cpp   ui/AppController.h
    ui/CaptureLibraryService.cpp ui/CaptureLibraryService.h
    ui/SettingsRouter.cpp ui/SettingsRouter.h
    ui/CurrentGameService.cpp ui/CurrentGameService.h
    ui/GalleryModel.cpp    ui/GalleryModel.h
    ui/ShellActions.cpp    ui/ShellActions.h
    # Win32 resource: embeds the multi-size app icon into GameHQ.exe.
    ${CMAKE_SOURCE_DIR}/assets/icons/gamehq.rc
)

set_source_files_properties(ui/qml/Theme.qml ui/qml/Brand.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(GameHQ
    URI GameHQ
    VERSION 1.0
    QML_FILES
        ui/qml/Main.qml
        ui/qml/OverlayWindow.qml
        ui/qml/ToastWindow.qml
        ui/qml/SettingsView.qml
        ui/qml/HelpView.qml
        ui/qml/Theme.qml
        ui/qml/Brand.qml
        ui/qml/helpers/SidebarCategories.js
        ui/qml/components/SidebarItem.qml
        ui/qml/components/DesktopSidebar.qml
        ui/qml/components/DesktopGalleryHeader.qml
        ui/qml/components/DesktopGalleryFooter.qml
        ui/qml/components/DesktopGalleryGrid.qml
        ui/qml/components/DesktopEmptyState.qml
        ui/qml/components/OverlaySidebar.qml
        ui/qml/components/OverlayCaptureStrip.qml
        ui/qml/components/OverlayPreview.qml
        ui/qml/components/OverlayFooter.qml
        ui/qml/components/OverlayActionMenu.qml
        ui/qml/components/CaptureTile.qml
        ui/qml/components/AccentButton.qml
        ui/qml/components/TileIconButton.qml
        ui/qml/components/ConfirmDialog.qml
        ui/qml/components/Toast.qml
        ui/qml/components/Lightbox.qml
        ui/qml/components/PlayerControls.qml
        ui/qml/components/NavRepeat.qml
        ui/qml/components/SettingsCombo.qml
        ui/qml/components/SettingsPage.qml
        ui/qml/components/SettingsSection.qml
        ui/qml/components/SettingsRow.qml
        ui/qml/components/SettingsToggle.qml
        ui/qml/components/SettingsCategoryButton.qml
        ui/qml/settings/GeneralSettingsPage.qml
        ui/qml/settings/CaptureSettingsPage.qml
        ui/qml/settings/ReplaySettingsPage.qml
        ui/qml/settings/InputSettingsPage.qml
        ui/qml/settings/LibrarySettingsPage.qml
        ui/qml/settings/FeedbackSettingsPage.qml
        ui/qml/settings/AdvancedSettingsPage.qml
    )

qt_add_resources(GameHQ "icons"
    PREFIX "/icons"
    BASE "${CMAKE_SOURCE_DIR}/assets/icons"
    FILES
        "${CMAKE_SOURCE_DIR}/assets/icons/gamehq.svg"
        "${CMAKE_SOURCE_DIR}/assets/icons/gamehq.ico"
)

qt_add_resources(GameHQ "sounds"
    PREFIX "/sounds"
    BASE "${CMAKE_SOURCE_DIR}/assets/sounds"
    FILES
        "${CMAKE_SOURCE_DIR}/assets/sounds/nav_tick.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/overlay_open.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/overlay_close.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/favorite.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/confirm.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/error.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/screenshot.wav"
        "${CMAKE_SOURCE_DIR}/assets/sounds/replay_saved.wav"
)

target_include_directories(GameHQ PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(GameHQ PRIVATE
    GAMEHQ_VERSION="${GAMEHQ_VERSION_FULL}"
)

target_link_libraries(GameHQ PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Sql Qt6::Widgets Qt6::Svg
    Qt6::Multimedia
    user32 gdi32 version winmm
    # 0.5 Windows Graphics Capture frame pump (WinRT raw-ABI path, no cppwinrt).
    windowsapp runtimeobject ole32 oleaut32 d3d11 dxgi dxguid uuid
    # 0.5 Step 4 H.264 encode via Media Foundation SinkWriter (fragmented MP4).
    mfplat mfreadwrite mfuuid mf avrt ksuser mmdevapi
)

set_target_properties(GameHQ PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Tiny static Win32 launcher for clean package roots (docs/packaging.md).
# The packaging scripts copy it to build/GameHQ.exe or dist/GameHQ/GameHQ.exe;
# the real executable and every runtime dependency live below app/.
add_executable(GameHQLauncher WIN32
    launcher/LauncherMain.cpp
    ${CMAKE_SOURCE_DIR}/assets/icons/gamehq.rc
)
target_link_options(GameHQLauncher PRIVATE -static)
