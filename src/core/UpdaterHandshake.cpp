#include "core/UpdaterHandshake.h"

#include <algorithm>
#include <cstdint>
#include <cwctype>

namespace handshake
{
std::wstring readyEventNameFor(const std::filesystem::path &transactionPath)
{
    std::wstring normalized = std::filesystem::absolute(transactionPath).lexically_normal().wstring();
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), towlower);
    std::uint64_t hash = 1469598103934665603ULL;
    for (wchar_t ch : normalized) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return L"Local\\GameHQUpdaterReady-" + std::to_wstring(hash);
}
} // namespace handshake
