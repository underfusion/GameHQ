#pragma once

#include <filesystem>
#include <string>

namespace handshake
{
// Named manual-reset event the updater helper signals once it has validated
// the transaction in --apply mode. The application creates and waits on the
// same event before it exits, so a helper that never starts or rejects the
// transaction is detected while the app can still cancel the update.
std::wstring readyEventNameFor(const std::filesystem::path &transactionPath);
} // namespace handshake
