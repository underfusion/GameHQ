#pragma once

#include <QString>

// Owns GameHQ's per-user Windows startup registration. The registry entry
// always targets the package launcher when one exists, so portable builds keep
// working after the real executable moved below app/.
class StartupManager
{
public:
    bool setEnabled(bool enabled) const;

private:
    static QString executablePath();
};
