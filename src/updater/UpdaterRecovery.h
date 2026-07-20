#pragma once
#include "updater/UpdaterTransaction.h"
#include <filesystem>
#include <string>

namespace updater
{
bool writeTransactionPhase(const Transaction &transaction, const std::string &phase,
                           std::string &errorOut);
bool applyUpdate(const Transaction &transaction, int healthTimeoutMs, std::string &errorOut);
bool recoverInterruptedUpdate(const Transaction &transaction, std::string &errorOut);
}
