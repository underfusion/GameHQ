#include "storage/CaptureQueries.h"
#include "config/Paths.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>

namespace
{
CaptureRecord recordFromQuery(const QSqlQuery& q)
{
    CaptureRecord r;
    r.id            = q.value(0).toInt();
    r.filePath      = Paths::repairMovedPath(q.value(1).toString());
    r.type          = q.value(2).toString();
    r.gameId        = q.value(3).isNull() ? -1 : q.value(3).toInt();
    r.gameName      = q.value(4).toString();
    r.createdAt     = q.value(5).toString();
    r.isFavorite    = q.value(6).toInt() != 0;
    r.thumbnailPath = Paths::repairMovedPath(q.value(7).toString());
    r.source        = q.value(8).toString();
    if (r.gameName.isEmpty())
        r.gameName = QStringLiteral("Unknown Game");
    return r;
}
} // namespace

QVector<CaptureRecord> CaptureQueries::listCaptures(const QSqlDatabase& db,
                                                    const QString& category,
                                                    int gameId)
{
    QString sql = QStringLiteral(
        "SELECT c.id, c.file_path, c.type, c.game_id, g.display_name, "
        "       c.created_at, c.is_favorite, c.thumbnail_path, c.source "
        "FROM captures c LEFT JOIN games g ON g.id = c.game_id "
        "WHERE c.deleted_at IS NULL");
    if (category == QLatin1String("favorites"))
        sql += QStringLiteral(" AND c.is_favorite = 1");
    else if (category == QLatin1String("screenshots"))
        sql += QStringLiteral(" AND c.type = 'screenshot'");
    else if (category == QLatin1String("clips"))
        sql += QStringLiteral(" AND c.type = 'video'");
    if (gameId >= 0)
        sql += QStringLiteral(" AND c.game_id = :game");
    sql += QStringLiteral(" ORDER BY c.created_at DESC");
    if (category == QLatin1String("recent"))
        sql += QStringLiteral(" LIMIT 50");

    QSqlQuery q(db);
    q.prepare(sql);
    if (gameId >= 0)
        q.bindValue(QStringLiteral(":game"), gameId);

    QVector<CaptureRecord> out;
    if (!q.exec()) {
        qWarning() << "DB: listCaptures failed:" << q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(recordFromQuery(q));
    return out;
}

bool CaptureQueries::hasCapture(const QSqlDatabase& db, const QString& filePath)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT 1 FROM captures WHERE file_path = :p"));
    q.bindValue(QStringLiteral(":p"), Paths::toStoredPath(filePath));
    return q.exec() && q.next();
}

bool CaptureQueries::hasCapturesForGame(const QSqlDatabase& db, int gameId)
{
    if (gameId < 0)
        return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM captures "
        "WHERE deleted_at IS NULL AND game_id = :game LIMIT 1"));
    q.bindValue(QStringLiteral(":game"), gameId);
    if (!q.exec()) {
        qWarning() << "DB: hasCapturesForGame failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

QVector<GameEntry> CaptureQueries::listGames(const QSqlDatabase& db)
{
    QVector<GameEntry> out;
    QSqlQuery q(QStringLiteral(
        "SELECT g.id, g.display_name, g.icon_path, g.executable_path, "
        "MAX(c.created_at) AS last_used FROM games g "
        "JOIN captures c ON c.game_id = g.id AND c.deleted_at IS NULL "
        "GROUP BY g.id, g.display_name, g.icon_path, g.executable_path "
        "ORDER BY last_used DESC"), db);
    while (q.next())
        out.append({ q.value(0).toInt(), q.value(1).toString(), Paths::repairMovedPath(q.value(2).toString()),
                     q.value(3).toString() });
    return out;
}
