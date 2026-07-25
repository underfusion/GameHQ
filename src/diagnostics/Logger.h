#pragma once
#include <QString>

// File logger. install() routes Qt logging (qInfo/qWarning/...) to
// <logsDir>/gamehq.log with timestamps, keeping console output in debug builds.
namespace Logger
{
    // A busy session writes a few hundred KiB, so eight MiB per file across
    // four files is plenty of history for a bug report while the log can never
    // grow without limit. Rotation keeps gamehq.log plus gamehq.1.log …
    // gamehq.3.log.
    inline constexpr qint64 kMaxLogBytes = 8LL * 1024 * 1024;
    inline constexpr int kRetainedLogs = 3;

    void install(const QString& logsDir);

    // True when the log file is open. A failed open is not fatal — messages go
    // to stderr instead — but the diagnostics summary should say so.
    bool writingToFile();

    // Rotates <directory>/<baseName> once it reaches maxBytes, keeping
    // `retained` previous generations and dropping the oldest. Renaming is
    // best effort: if a generation cannot be moved (something still has it
    // open), the current file is left alone and false is returned — letting the
    // log grow past the limit beats losing what is in it.
    bool rotateIfNeeded(const QString& directory, const QString& baseName,
                        qint64 maxBytes, int retained);
}
