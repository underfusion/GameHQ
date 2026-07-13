#pragma once
#include <QObject>
#include <QSystemTrayIcon>

class QMenu;

// Tray icon + menu (docs/product-spec.md §16). Capture actions appear as
// disabled placeholders until milestones 0.4/0.5 wire them up.
class TrayIcon : public QObject
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject* parent = nullptr);
    ~TrayIcon() override;

    // Balloon notification (docs/product-spec.md §16); no-op if unsupported.
    void showNotification(const QString& title, const QString& body);

signals:
    void openGalleryRequested();
    void rescanRequested();
    void screenshotRequested();
    void quitRequested();

private:
    QSystemTrayIcon* m_tray;
    QMenu* m_menu;
};
