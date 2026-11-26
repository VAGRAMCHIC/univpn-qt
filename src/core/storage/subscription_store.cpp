#include "subscription_store.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>

SubscriptionStore::SubscriptionStore(QString filePath)
    : m_filePath(std::move(filePath)) {}

QList<Subscription> SubscriptionStore::load() const
{
    QFile file(m_filePath);
    if (!file.exists())
        return {};

    if (!file.open(QIODevice::ReadOnly))
        return {};

    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    const auto items = doc.object().value(QStringLiteral("subscriptions"));
    if (!items.isArray())
        return {};

    QList<Subscription> result;
    for (const auto &v : items.toArray()) {
        if (!v.isObject()) continue;
        const auto sub = Subscription::fromJson(v.toObject());
        if (sub.has_value())
            result.append(*sub);
    }
    return result;
}

bool SubscriptionStore::save(const QList<Subscription> &subscriptions) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray arr;
    for (const auto &s : subscriptions)
        arr.append(s.toJson());

    QJsonObject root;
    root[QStringLiteral("version")]       = 1;
    root[QStringLiteral("subscriptions")] = arr;

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
