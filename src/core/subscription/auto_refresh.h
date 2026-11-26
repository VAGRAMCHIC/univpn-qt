#pragma once

#include <QObject>
#include <QTimer>

#include "subscription.h"

class SubscriptionsManager;

/// Пора ли обновлять подписку: с последнего обновления прошёл её интервал.
bool isDueForRefresh(const Subscription &subscription, const QDateTime &now);
/// Фоновое обновление подписок.
///
/// Проверяет расписание раз в checkEvery; упавшая подписка не мешает остальным
/// и не стирает уже загруженные узлы.
class SubscriptionAutoRefresh : public QObject {
    Q_OBJECT

public:
    explicit SubscriptionAutoRefresh(SubscriptionsManager *manager,
                                     QObject *parent = nullptr);

    void start();
    void stop();

private:
    void check();

    SubscriptionsManager *m_manager;
    QTimer m_timer;
};
