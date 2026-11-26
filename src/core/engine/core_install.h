#pragma once

#include <QString>
#include <optional>

/// Где лежит ядро и наборы правил.
struct CoreInstall {
    QString binary;
    std::optional<QString> ruleSetsDir;

    static constexpr const char *envBinary = "CLGVPN_SINGBOX";

    /// Ищет ядро рядом с приложением, в переменной окружения и в core/dist.
    /// Бросает std::runtime_error, если не нашёл.
    static CoreInstall resolve();
};
