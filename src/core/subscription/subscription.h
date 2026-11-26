#pragma once

#include <QString>
#include <QDateTime>
#include <QList>
#include <QJsonObject>
#include <optional>

#include "../model/vpn_node.h"

/// Сколько трафика осталось по подписке (заголовок `subscription-userinfo`).
struct SubscriptionTraffic {
    qint64 upload = 0;
    qint64 download = 0;
    qint64 total = 0;              // 0 — безлимит
    std::optional<QDateTime> expire;

    qint64 used() const { return upload + download; }
    bool isUnlimited() const { return total <= 0; }
    bool isExpired() const;

    QJsonObject toJson() const;
    static SubscriptionTraffic fromJson(const QJsonObject &json);
};

/// Подписка: ссылка, узлы и метаданные из заголовков ответа.
struct Subscription {
    QString id;
    QString url;
    QString title;
    QList<VpnNode> nodes;
    QDateTime updatedAt;
    std::optional<SubscriptionTraffic> traffic;
    int updateIntervalMinutes = 720;   // 12 часов

    QJsonObject toJson() const;
    static std::optional<Subscription> fromJson(const QJsonObject &json);
};

/// Стабильный идентификатор подписки: хеш URL.
QString subscriptionIdFor(const QString &url);
