#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace maintenance
{
enum class State { Inactive, Active, StaleRecovery };

struct Info
{
    State state = State::Inactive;
    std::string phase;
};

bool begin(const std::filesystem::path &packageRoot, std::string &error);
void finish(const std::filesystem::path &packageRoot);
Info inspect(const std::filesystem::path &packageRoot);
Info inspect(const std::filesystem::path &packageRoot, bool helperActive,
             std::filesystem::file_time_type now,
             std::chrono::seconds staleAfter = std::chrono::minutes(5));
} // namespace maintenance
