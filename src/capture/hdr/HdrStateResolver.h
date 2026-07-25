#pragma once

#include <optional>

namespace capture::hdr
{
// Windows' active color mode is the authoritative state of the HDR toggle.
// DXGI's colour space can lag behind after a toggle on hybrid-GPU/display
// paths, so use it only when DisplayConfig could not report a mode.
inline bool resolveHdrActive(bool dxgiPqActive,
                             const std::optional<bool>& windowsHdrActive)
{
    return windowsHdrActive.value_or(dxgiPqActive);
}
} // namespace capture::hdr
