#pragma once

#include <QObject>
#include <QList>
#include "subscription.h"

class SubscriptionService;
class SubscriptionStore;

/// Менеджер подписок: загрузка с диска, добавление, обновление, удаление.
/// Заменяет SubscriptionsNotifier из Flutter-версии.
class SubscriptionsManager : public QObject {
    Q_OBJECT

public:
    SubscriptionsManager(SubscriptionService *service,
                         SubscriptionStore *store,
                         QObject *parent = nullptr);

    const QList<Subscription> &subscriptions() const { return m_subscriptions; }

    void loadFromStore();

    /// Асинхронно. При успехе — сигналы subscriptionAdded/subscriptionsChanged.
    void add(const QString &url);

    /// Асинхронно. При успехе — subscriptionsChanged.
    void refresh(const QString &id);

    void remove(const QString &id);

    const Subscription *findById(const QString &id) const;
    QList<VpnNode> allNodes() const;

signals:
    void subscriptionsChanged();
    void subscriptionAdded(const Subscription &subscription);
    void errorOccurred(const QString &message);

private:
    void replaceAll(const QList<Subscription> &next);
    void persist();

    SubscriptionService *m_service;
    SubscriptionStore *m_store;
    QList<Subscription> m_subscriptions;
};
