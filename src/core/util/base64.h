#pragma once

#include <QString>
#include <optional>

/// Декодирует base64/base64url с переносами строк и без паддинга.
/// Возвращает std::nullopt, если строка не является корректным base64.
std::optional<QString> tryDecodeBase64(const QString &value);
