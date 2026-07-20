#pragma once

#include "updater/UpdaterTransaction.h"

#include <string>

namespace updater
{
bool extractAndValidatePackage(const Transaction &transaction, std::string &errorOut);
} // namespace updater
