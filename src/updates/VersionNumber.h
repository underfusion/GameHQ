#pragma once

#include <QString>
#include <optional>

// Strict major.minor.patch version parsing and comparison for update checks.
// Versions are NEVER compared as strings ("9" < "10" lexically, but 9 < 10
// numerically is what every caller actually wants).
class VersionNumber
{
public:
    // Accepts "1.2.3" or "v1.2.3" / "V1.2.3". Each component must be "0" or
    // a digit sequence with no leading zero, and nothing may follow the
    // patch number (no build metadata, no prerelease suffix). Returns
    // std::nullopt for anything else.
    static std::optional<VersionNumber> parse(const QString &text);

    int major() const { return m_major; }
    int minor() const { return m_minor; }
    int patch() const { return m_patch; }

    QString toString() const;

    friend bool operator==(const VersionNumber &a, const VersionNumber &b);
    friend bool operator!=(const VersionNumber &a, const VersionNumber &b);
    friend bool operator<(const VersionNumber &a, const VersionNumber &b);
    friend bool operator>(const VersionNumber &a, const VersionNumber &b);
    friend bool operator<=(const VersionNumber &a, const VersionNumber &b);
    friend bool operator>=(const VersionNumber &a, const VersionNumber &b);

private:
    VersionNumber(int major, int minor, int patch);

    int m_major = 0;
    int m_minor = 0;
    int m_patch = 0;
};
