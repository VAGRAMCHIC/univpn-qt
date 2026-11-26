#pragma once

#include <QString>
#include <QJsonObject>
#include <optional>

#include "../singbox/config_builder.h"

/// Пользовательские настройки, которые переживают перезапуск.
struct AppSettings {
    RoutingMode routingMode = RoutingMode::Smart;
    bool autoConnect = false;

    /// nullopt — как в системе, иначе "ru" / "en".
    std::optional<QString> localeCode;

    std::optional<QString> selectedNodeTag;

    /// Блокировать трафик мимо VPN (nftables, только в режиме TUN).
    bool killSwitch = false;

    QJsonObject toJson() const;
    static AppSettings fromJson(const QJsonObject &json);
};
