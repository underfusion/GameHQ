#pragma once
#include <filesystem>

namespace launcher
{
bool promotePendingUpdater(const std::filesystem::path &packageRoot);
}
