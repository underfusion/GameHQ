#include "storage/GameRowRepair.h"
#include "core/GameIdentity.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVector>

bool GameRowRepair::isBetterDisplayName(const QString& candidate, const QString& current)
{
    if (current.isEmpty())
        return true;
    if (GameIdentity::hasFolderForbiddenChar(candidate) != GameIdentity::hasFolderForbiddenChar(current))
        return GameIdentity::hasFolderForbiddenChar(candidate);
    return candidate.size() > current.size();
}

void GameRowRepair::normalizeDuplicateNames(QSqlDatabase& db)
{
    struct Row { int id; QString name; };
    QVector<Row> rows;

    QSqlQuery q(QStringLiteral("SELECT id, display_name FROM games ORDER BY id"), db);
    while (q.next())
        rows.append({ q.value(0).toInt(), q.value(1).toString() });

    for (int i = 0; i < rows.size(); ++i) {
        if (rows.at(i).id < 0)
            continue;

        int preferredId = rows.at(i).id;
        QString preferredName = rows.at(i).name;
        const QString key = GameIdentity::key(preferredName);

        for (int j = i + 1; j < rows.size(); ++j) {
            if (rows.at(j).id < 0 || GameIdentity::key(rows.at(j).name) != key)
                continue;

            int duplicateId = rows.at(j).id;
            QString duplicateName = rows.at(j).name;
            if (isBetterDisplayName(duplicateName, preferredName)) {
                duplicateId = preferredId;
                duplicateName = preferredName;
                preferredId = rows.at(j).id;
                preferredName = rows.at(j).name;
            }

            QSqlQuery updateCaptures(db);
            updateCaptures.prepare(QStringLiteral(
                "UPDATE captures SET game_id = :keep WHERE game_id = :drop"));
            updateCaptures.bindValue(QStringLiteral(":keep"), preferredId);
            updateCaptures.bindValue(QStringLiteral(":drop"), duplicateId);
            if (!updateCaptures.exec()) {
                qWarning() << "DB: could not merge duplicate game captures:"
                           << updateCaptures.lastError().text();
                continue;
            }

            QSqlQuery deleteGame(db);
            deleteGame.prepare(QStringLiteral("DELETE FROM games WHERE id = :id"));
            deleteGame.bindValue(QStringLiteral(":id"), duplicateId);
            if (!deleteGame.exec()) {
                qWarning() << "DB: could not delete duplicate game:"
                           << deleteGame.lastError().text();
                continue;
            }

            qInfo() << "DB: merged duplicate game" << duplicateName
                    << "into" << preferredName;
            for (Row& row : rows) {
                if (row.id == duplicateId)
                    row.id = -1;
            }
        }

        if (preferredId != rows.at(i).id) {
            rows[i].id = -1;
            rows.append({ preferredId, preferredName });
        }
    }
}
