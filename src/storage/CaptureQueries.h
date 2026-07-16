#pragma once

#include "storage/CaptureDatabase.h"

#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

namespace CaptureQueries
{
QVector<CaptureRecord> listCaptures(const QSqlDatabase& db, const QString& category, int gameId);
bool hasCapture(const QSqlDatabase& db, const QString& filePath);
// One-shot snapshot of every captures row, keyed by the stored (normalized)
// file path, for callers that would otherwise probe the table per file.
// Tombstoned rows are included so existence checks match hasCapture(); their
// thumbnail is reported empty, matching thumbnailForCapture()'s deleted_at filter.
QHash<QString, CaptureIndexEntry> captureIndex(const QSqlDatabase& db);
bool hasCapturesForGame(const QSqlDatabase& db, int gameId);
QVector<GameEntry> listGames(const QSqlDatabase& db);
} // namespace CaptureQueries
