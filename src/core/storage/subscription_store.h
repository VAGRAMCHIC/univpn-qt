#pragma once

#include <QString>
#include <QList>
#include "../subscription/subscription.h"

/// Хранит подписки в JSON-файле рядом с настройками приложения.
///
/// Формат совпадает с Flutter-версией:
/// { "version": 1, "subscriptions": [ ... ] }
class SubscriptionStore {
public:
    explicit SubscriptionStore(QString filePath);

    QList<Subscription> load() const;
    bool save(const QList<Subscription> &subscriptions) const;

    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};
