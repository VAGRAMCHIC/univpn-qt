#include "subscription_manager.h"
#include "subscription_service.h"
#include "../storage/subscription_store.h"

SubscriptionsManager::SubscriptionsManager(SubscriptionService *service,
                                           SubscriptionStore *store,
                                           QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_store(store)
{
    connect(m_service, &SubscriptionService::finished,
            this, [this](const Subscription &sub) {
                // add() и refresh() различаются: для add() заменяем по id,
                // для refresh() — тоже. Логика одна.
                QList<Subscription> next = m_subscriptions;
                bool replaced = false;
                for (auto &item : next) {
                    if (item.id == sub.id) { item = sub; replaced = true; break; }
                }
                if (!replaced) next.append(sub);
                replaceAll(next);
                if (!replaced) emit subscriptionAdded(sub);
            });

    connect(m_service, &SubscriptionService::failed,
            this, &SubscriptionsManager::errorOccurred);
}

void SubscriptionsManager::loadFromStore()
{
    m_subscriptions = m_store->load();
    emit subscriptionsChanged();
}

void SubscriptionsManager::add(const QString &url)
{
    m_service->fetch(url);
}

void SubscriptionsManager::refresh(const QString &id)
{
    const auto *existing = findById(id);
    if (!existing) return;
    m_service->fetch(existing->url);
}

void SubscriptionsManager::remove(const QString &id)
{
    QList<Subscription> next;
    for (const auto &s : m_subscriptions)
        if (s.id != id) next.append(s);
    replaceAll(next);
}

const Subscription *SubscriptionsManager::findById(const QString &id) const
{
    for (const auto &s : m_subscriptions)
        if (s.id == id) return &s;
    return nullptr;
}

QList<VpnNode> SubscriptionsManager::allNodes() const
{
    QList<VpnNode> result;
    for (const auto &s : m_subscriptions)
        result.append(s.nodes);
    return result;
}

void SubscriptionsManager::replaceAll(const QList<Subscription> &next)
{
    m_subscriptions = next;
    persist();
    emit subscriptionsChanged();
}

void SubscriptionsManager::persist()
{
    m_store->save(m_subscriptions);
}
