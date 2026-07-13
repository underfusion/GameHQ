#pragma once

#include <QString>

class ShellActions
{
public:
    static void openFile(const QString& filePath);
    static void showInFolder(const QString& filePath);
};
