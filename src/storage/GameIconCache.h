#pragma once

#include <QString>

class GameIconCache
{
public:
    static QString iconPathForExecutable(const QString& executablePath);
};
