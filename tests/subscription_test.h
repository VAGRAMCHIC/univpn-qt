#pragma once

#include <QObject>

class SubscriptionTest : public QObject {
    Q_OBJECT

private slots:
    void vlessReality();
    void vlessInvalidUuid();
    void trojanPassword();
    void shadowsocksBase64();
    void shadowsocksUserAtHost();
    void shadowsocksInvalid();
    void bodyFromPlainList();
    void bodyFromBase64();
    void bodyRejectsJson();
    void bodySkipsBrokenLines();
    void metaFromHeaders();
    void dueForRefresh();
};
