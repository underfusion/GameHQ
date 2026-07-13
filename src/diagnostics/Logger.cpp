#include "diagnostics/Logger.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QTextStream>

namespace
{
QFile g_logFile;
QMutex g_mutex;
QtMessageHandler g_previous = nullptr;

const char* levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return "DBG";
    case QtInfoMsg:     return "INF";
    case QtWarningMsg:  return "WRN";
    case QtCriticalMsg: return "ERR";
    case QtFatalMsg:    return "FTL";
    }
    return "???";
}

void handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    {
        QMutexLocker lock(&g_mutex);
        if (g_logFile.isOpen()) {
            QTextStream out(&g_logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                << ' ' << levelName(type) << ' ' << msg << '\n';
            out.flush();
        }
    }
#ifdef QT_DEBUG
    if (g_previous)
        g_previous(type, ctx, msg);
#else
    Q_UNUSED(ctx);
#endif
}
} // namespace

namespace Logger
{

void install(const QString& logsDir)
{
    QMutexLocker lock(&g_mutex);
    g_logFile.setFileName(logsDir + QStringLiteral("/gamehq.log"));
    g_logFile.open(QIODevice::Append | QIODevice::Text);
    lock.unlock();
    g_previous = qInstallMessageHandler(handler);
}

} // namespace Logger
