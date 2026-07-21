#pragma once

#include <QString>

class GameIconCache
{
public:
    static QString iconPathForExecutable(const QString& executablePath);

    // Bumped whenever the extractor learns to produce a better icon. Callers
    // that pinned an icon path persist this alongside it, so they can tell a
    // stale icon from a current one without re-reading every executable.
    static QString formatVersion();
};
