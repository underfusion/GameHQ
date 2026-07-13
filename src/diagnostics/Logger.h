#pragma once
#include <QString>

// File logger. install() routes Qt logging (qInfo/qWarning/...) to
// <logsDir>/gamehq.log with timestamps, keeping console output in debug builds.
namespace Logger
{
    void install(const QString& logsDir);
}
