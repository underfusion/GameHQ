#pragma once
#include "updater/UpdaterTransaction.h"
#include <string>
namespace updater {
bool swapProgramFiles(const Transaction &transaction, std::string &errorOut);
bool rollbackProgramFiles(const Transaction &transaction, std::string &errorOut);
bool hasProgramSwapJournal(const Transaction &transaction);
}
