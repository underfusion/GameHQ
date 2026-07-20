#pragma once

#include <QDateTime>
#include <QString>

// Immutable snapshot of one GitHub release, already filtered down to the
// fields the update checker needs. Never carries an access token.
struct ReleaseInfo
{
    QString version;      // Tag name, expected to equal the VERSION file exactly.
    QString name;          // Release title.
    QString notes;         // Release body (raw Markdown/plain text from GitHub).
    QDateTime publishedAt;
    QString webUrl;        // Human-facing GitHub release page.
    QString zipUrl;        // Download URL of the "-win64-update.zip" asset.
    QString zipName;
    qint64 zipSize = 0;
    QString checksumUrl;   // Download URL of the matching ".sha256" asset.
    bool prerelease = false;
    bool draft = false;
};
