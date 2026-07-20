#pragma once
#include "updater/UpdaterTransaction.h"
#include <string>

namespace updater
{
bool launchAndWaitForHealth(const Transaction &transaction, int timeoutMs,
                            std::string &errorOut);
}
