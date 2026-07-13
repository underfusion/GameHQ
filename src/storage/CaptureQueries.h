#pragma once

#include "storage/CaptureDatabase.h"

#include <QSqlDatabase>
#include <QString>
#include <QVector>

namespace CaptureQueries
{
QVector<CaptureRecord> listCaptures(const QSqlDatabase& db, const QString& category, int gameId);
bool hasCapture(const QSqlDatabase& db, const QString& filePath);
bool hasCapturesForGame(const QSqlDatabase& db, int gameId);
QVector<GameEntry> listGames(const QSqlDatabase& db);
} // namespace CaptureQueries
