#pragma once

#include <QJsonObject>
#include <QString>

namespace integration
{
struct Message
{
    QString type;
    QString requestId;
    QJsonObject object;
};
} // namespace integration
