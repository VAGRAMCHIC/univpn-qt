#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <optional>
#include "../model/vpn_node.h"
#include "subscription.h"

/// Метаданные, которые панель отдаёт заголовками.
struct SubscriptionMeta {
    std::optional<QString> title;
    std::optional<SubscriptionTraffic> traffic;
    std::optional<int> updateIntervalHours;

    static SubscriptionMeta parse(const QMap<QString, QString> &headers);
};

/// Разбирает тело подписки: список ссылок, base64-список или sing-box JSON.
/// Бросает std::invalid_argument на JSON-подписках (как в Dart).
QList<VpnNode> parseSubscriptionBody(const QString &raw);
