#pragma once

#include <QString>

namespace GameIdentity
{
QString folderName(const QString& name);
QString key(const QString& name);
bool hasFolderForbiddenChar(const QString& name);

// Infers which game a capture belongs to from its location under `root`:
// "<root>/<Game>/Screenshots/shot.png" (or .../Clips/clip.mp4) and
// "<root>/<Game>/shot.png" both yield "<Game>". A file sitting directly in the
// root has no game folder to read, so it gets the same fallback name as
// folderName(). Pure path arithmetic — touches no disk.
QString inferFromPath(const QString& root, const QString& filePath);
}
