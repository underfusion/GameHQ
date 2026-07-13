#pragma once

#include <QSqlDatabase>
#include <QString>

class GameRowRepair
{
public:
    static bool isBetterDisplayName(const QString& candidate, const QString& current);
    static void normalizeDuplicateNames(QSqlDatabase& db);
};
