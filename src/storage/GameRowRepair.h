#pragma once

#include <QSqlDatabase>
#include <QString>

class GameRowRepair
{
public:
    static bool isBetterDisplayName(const QString& candidate, const QString& current);
    // Returns false when a merge could not be completed, so the caller can roll
    // the whole startup repair back instead of committing a half-merged library.
    static bool normalizeDuplicateNames(QSqlDatabase& db);
};
