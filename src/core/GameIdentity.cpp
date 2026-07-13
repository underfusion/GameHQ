#include "core/GameIdentity.h"

namespace
{
constexpr auto kFallbackName = "Unknown Game";
constexpr auto kForbiddenChars = "<>:\"/\\|?*";
}

namespace GameIdentity
{
QString folderName(const QString& name)
{
    const QString forbidden = QString::fromLatin1(kForbiddenChars);
    QString out;
    out.reserve(name.size());
    for (const QChar c : name)
        out += forbidden.contains(c) ? QLatin1Char('_') : c;
    out = out.trimmed();
    return out.isEmpty() ? QString::fromLatin1(kFallbackName) : out;
}

QString key(const QString& name)
{
    return folderName(name).simplified().toCaseFolded();
}

bool hasFolderForbiddenChar(const QString& name)
{
    const QString forbidden = QString::fromLatin1(kForbiddenChars);
    for (const QChar c : name) {
        if (forbidden.contains(c))
            return true;
    }
    return false;
}
}
