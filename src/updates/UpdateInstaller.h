#pragma once
#include "updates/ReleaseInfo.h"

#include <QByteArray>
#include <QString>

namespace UpdateInstaller
{
// Writes the update transaction, including the signed-manifest evidence the
// helper re-checks before it touches the archive.
bool prepareTransaction(const QString &packageRoot, const QString &dataDir,
                        const VerifiedUpdate &verified, QString &transactionPathOut,
                        QString &errorOut);
bool launchPrepared(const QString &packageRoot, const QString &transactionPath,
                    QString &errorOut);
}
