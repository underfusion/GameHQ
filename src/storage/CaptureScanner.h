#pragma once
#include "storage/CaptureDatabase.h"

#include <QHash>
#include <QObject>
#include <QString>

class CaptureLocations;

// Scans the managed captures root + watched folders for media files and
// registers new ones in the database (with thumbnails for images).
// Game inference: <root>/<Game>/(Screenshots|Clips)/file → "<Game>";
// otherwise the file's parent folder name; fallback "Unknown Game".
class CaptureScanner : public QObject
{
    Q_OBJECT
public:
    CaptureScanner(CaptureDatabase* db, CaptureLocations* locations,
                   QString thumbnailsDir, QObject* parent = nullptr);

    // Synchronous full scan; returns number of newly added captures.
    // Called at startup and from the UI "Rescan" action; move to a worker
    // thread once libraries grow beyond a few thousand files.
    int scanAll();

signals:
    void scanFinished(int added);

private:
    // index is the whole-table snapshot taken by scanAll(); scanFolder reads it
    // instead of querying per file and keeps it current as it inserts.
    int scanFolder(const QString& root, const QString& source,
                   QHash<QString, CaptureIndexEntry>& index);

    CaptureDatabase* m_db;
    CaptureLocations* m_locations;
    QString m_thumbnailsDir;
};
