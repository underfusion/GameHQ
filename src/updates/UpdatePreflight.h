#pragma once
#include <QString>

namespace UpdatePreflight
{
bool check(const QString &packageRoot, qint64 downloadBytes, QString &errorOut);
}
