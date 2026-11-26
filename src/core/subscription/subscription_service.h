#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include "subscription.h"

/// Загружает подписку по URL и превращает её в Subscription.
class SubscriptionService : public QObject {
    Q_OBJECT

public:
    explicit SubscriptionService(QObject *parent = nullptr);

    /// Асинхронная загрузка. Сигналы:
    ///   finished(Subscription) — при успехе
    ///   failed(QString)        — при ошибке (сообщение для пользователя)
    void fetch(const QString &rawUrl);

signals:
    void finished(Subscription subscription);
    void failed(const QString &message);

private:
    QNetworkAccessManager m_network;
};
