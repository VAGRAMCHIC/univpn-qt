#include "subscription.h"
#include "../util/stable_hash.h"

QJsonObject SubscriptionTraffic::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("upload")]   = static_cast<double>(upload);
    o[QStringLiteral("download")] = static_cast<double>(download);
    o[QStringLiteral("total")]    = static_cast<double>(total);
    if (expire.has_value())
        o[QStringLiteral("expire")] = expire->toString(Qt::ISODate);
    return o;
}

SubscriptionTraffic SubscriptionTraffic::fromJson(const QJsonObject &json)
{
    SubscriptionTraffic t;
    t.upload   = static_cast<qint64>(json.value(QStringLiteral("upload")).toDouble());
    t.download = static_cast<qint64>(json.value(QStringLiteral("download")).toDouble());
    t.total    = static_cast<qint64>(json.value(QStringLiteral("total")).toDouble());

    const QString expireStr = json.value(QStringLiteral("expire")).toString();
    if (!expireStr.isEmpty()) {
        const QDateTime dt = QDateTime::fromString(expireStr, Qt::ISODate);
        if (dt.isValid())
            t.expire = dt;
    }
    return t;
}

bool SubscriptionTraffic::isExpired() const
{
    return expire.has_value() && expire->isValid()
           && *expire < QDateTime::currentDateTime();
}

QJsonObject Subscription::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("id")]        = id;
    o[QStringLiteral("url")]       = url;
    o[QStringLiteral("title")]     = title;

    QJsonArray nodesArr;
    for (const auto &node : nodes)
        nodesArr.append(node.toJson());
    o[QStringLiteral("nodes")] = nodesArr;

    o[QStringLiteral("updatedAt")] = updatedAt.toUTC().toString(Qt::ISODate);

    if (traffic.has_value())
        o[QStringLiteral("traffic")] = traffic->toJson();

    o[QStringLiteral("updateIntervalMinutes")] = updateIntervalMinutes;
    return o;
}

std::optional<Subscription> Subscription::fromJson(const QJsonObject &json)
{
    Subscription s;
    s.id    = json.value(QStringLiteral("id")).toString();
    s.url   = json.value(QStringLiteral("url")).toString();
    s.title = json.value(QStringLiteral("title")).toString();

    if (s.id.isEmpty() || s.url.isEmpty())
        return std::nullopt;

    const auto nodesArr = json.value(QStringLiteral("nodes")).toArray();
    for (const auto &v : nodesArr) {
        if (!v.isObject()) continue;
        const auto node = VpnNode::fromJson(v.toObject());
        if (node.has_value())
            s.nodes.append(*node);
    }

    s.updatedAt = QDateTime::fromString(
        json.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
    if (!s.updatedAt.isValid())
        s.updatedAt = QDateTime::fromSecsSinceEpoch(0);

    if (json.contains(QStringLiteral("traffic")) &&
        json.value(QStringLiteral("traffic")).isObject()) {
        s.traffic = SubscriptionTraffic::fromJson(
            json.value(QStringLiteral("traffic")).toObject());
    }

    s.updateIntervalMinutes = json.value(QStringLiteral("updateIntervalMinutes"))
                                  .toInt(720);
    return s;
}

QString subscriptionIdFor(const QString &url)
{
    return QStringLiteral("s-") + stableHash32(url);
}
