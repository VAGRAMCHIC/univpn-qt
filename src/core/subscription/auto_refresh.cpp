#include "auto_refresh.h"
#include "subscription_manager.h"

bool isDueForRefresh(const Subscription &subscription, const QDateTime &now)
{
    return subscription.updatedAt.secsTo(now)
           >= static_cast<qint64>(subscription.updateIntervalMinutes) * 60;
}

SubscriptionAutoRefresh::SubscriptionAutoRefresh(SubscriptionsManager *manager,
                                                 QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
    m_timer.setInterval(5 * 60 * 1000);   // 5 минут
    connect(&m_timer, &QTimer::timeout, this, &SubscriptionAutoRefresh::check);
}

void SubscriptionAutoRefresh::start()
{
    m_timer.start();
    check();
}

void SubscriptionAutoRefresh::stop()
{
    m_timer.stop();
}

void SubscriptionAutoRefresh::check()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const auto &sub : m_manager->subscriptions()) {
        if (!isDueForRefresh(sub, now)) continue;
        m_manager->refresh(sub.id);
    }
}
