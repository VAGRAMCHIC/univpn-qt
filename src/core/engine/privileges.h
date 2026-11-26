// privileges.h
#pragma once

#include <QString>

/// Может ли приложение поднять TUN без запроса root.
/// Linux: проверяем файловые capabilities на бинаре ядра.
/// Windows: false (TUN требует прав администратора).
class Privileges {
public:
    static bool canCreateTun(const QString &binaryPath,
                             const QString &getcap = QStringLiteral("getcap"));
};
