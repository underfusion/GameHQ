#pragma once

namespace SonyReportLayout
{
enum class Family {
    DualSense,
    Ds4,
};

// The button block follows the left-stick axes by four bytes in DS4 reports
// and by seven bytes in DualSense reports. Both USB and Bluetooth layouts
// preserve that family-specific distance while shifting the whole payload.
inline int stickAxisBase(Family family, int buttonBlockBase)
{
    return buttonBlockBase - (family == Family::Ds4 ? 4 : 7);
}
} // namespace SonyReportLayout
