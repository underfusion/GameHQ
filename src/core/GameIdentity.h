#pragma once

#include <QString>

namespace GameIdentity
{
QString folderName(const QString& name);
QString key(const QString& name);
bool hasFolderForbiddenChar(const QString& name);
}
