#include "tray/TrayIcon.h"
#include "Brand.h"

#include <QAction>
#include <QIcon>
#include <QMenu>

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
    , m_menu(new QMenu)
{
    QAction* open = m_menu->addAction(tr("Open Gallery"));
    connect(open, &QAction::triggered, this, &TrayIcon::openGalleryRequested);

    QAction* rescan = m_menu->addAction(tr("Rescan Captures"));
    connect(rescan, &QAction::triggered, this, &TrayIcon::rescanRequested);

    m_menu->addSeparator();
    QAction* shot = m_menu->addAction(tr("Take Screenshot"));       // milestone 0.4
    connect(shot, &QAction::triggered, this, &TrayIcon::screenshotRequested);
    m_menu->addAction(tr("Save Replay"))->setEnabled(false);       // milestone 0.5
    m_menu->addSeparator();

    QAction* quit = m_menu->addAction(tr("Exit"));
    connect(quit, &QAction::triggered, this, &TrayIcon::quitRequested);

    m_tray->setIcon(QIcon(QStringLiteral(":/icons/gamehq.ico")));
    m_tray->setToolTip(QString::fromLatin1(Brand::Name) + QLatin1Char(' ')
                       + QStringLiteral(GAMEHQ_VERSION));
    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick)
                    emit openGalleryRequested();
            });
    m_tray->show();
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::showNotification(const QString& title, const QString& body)
{
    if (QSystemTrayIcon::supportsMessages())
        m_tray->showMessage(title, body, QIcon(QStringLiteral(":/icons/gamehq.ico")), 3000);
}
