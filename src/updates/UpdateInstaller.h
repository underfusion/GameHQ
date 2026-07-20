#pragma once
#include <QByteArray>
#include <QString>

namespace UpdateInstaller
{
bool prepareTransaction(const QString &packageRoot, const QString &dataDir,
                        const QString &packagePath, const QString &version,
                        const QByteArray &sha256, QString &transactionPathOut,
                        QString &errorOut);
bool launchPrepared(const QString &packageRoot, const QString &transactionPath,
                    QString &errorOut);
}
