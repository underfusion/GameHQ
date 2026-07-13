#pragma once

#include <QSqlDatabase>

class GameMetadataBackfill
{
public:
    static void run(QSqlDatabase& db);
};
