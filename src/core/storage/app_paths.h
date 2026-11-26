#pragma once

#include <QString>

/// Каталог приложения для конфигов, кэша ядра и подписок.
///
/// Аналог appSupportDirProvider из Flutter-версии:
/// Linux:   ~/.local/share/clgvpn/core
/// Windows: %APPDATA%/clgvpn/core
QString appSupportDir();

/// Гарантирует существование каталога (создаёт рекурсивно).
/// Возвращает путь или пустую строку, если создать не удалось.
QString ensureAppSupportDir();
