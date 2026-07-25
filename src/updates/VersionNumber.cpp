#include "updates/VersionNumber.h"

#include <QRegularExpression>
#include <tuple>

namespace
{
// Leading v/V, then three dot-separated numeric components, each either "0"
// or a digit sequence with no leading zero. Nothing may trail the patch
// number, so "1.2.3-beta" and "1.2.3.4" are both rejected as malformed.
const QRegularExpression &versionPattern()
{
    static const QRegularExpression pattern(
        QStringLiteral("^[vV]?(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)$"));
    return pattern;
}
} // namespace

VersionNumber::VersionNumber(int major, int minor, int patch)
    : m_major(major), m_minor(minor), m_patch(patch)
{
}

std::optional<VersionNumber> VersionNumber::parse(const QString &text)
{
    const QRegularExpressionMatch match = versionPattern().match(text);
    if (!match.hasMatch())
        return std::nullopt;

    bool okMajor = false;
    bool okMinor = false;
    bool okPatch = false;
    const int major = match.captured(1).toInt(&okMajor);
    const int minor = match.captured(2).toInt(&okMinor);
    const int patch = match.captured(3).toInt(&okPatch);
    if (!okMajor || !okMinor || !okPatch)
        return std::nullopt;

    return VersionNumber(major, minor, patch);
}

QString VersionNumber::toString() const
{
    return QStringLiteral("%1.%2.%3").arg(m_major).arg(m_minor).arg(m_patch);
}

bool operator==(const VersionNumber &a, const VersionNumber &b)
{
    return a.m_major == b.m_major && a.m_minor == b.m_minor && a.m_patch == b.m_patch;
}

bool operator!=(const VersionNumber &a, const VersionNumber &b)
{
    return !(a == b);
}

bool operator<(const VersionNumber &a, const VersionNumber &b)
{
    return std::tie(a.m_major, a.m_minor, a.m_patch) < std::tie(b.m_major, b.m_minor, b.m_patch);
}

bool operator>(const VersionNumber &a, const VersionNumber &b)
{
    return b < a;
}

bool operator<=(const VersionNumber &a, const VersionNumber &b)
{
    return !(b < a);
}

bool operator>=(const VersionNumber &a, const VersionNumber &b)
{
    return !(a < b);
}
