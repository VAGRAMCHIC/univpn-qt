// ip_check.h
#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <optional>

/// Запрашивает внешний IP, чтобы показать, что трафик идёт через туннель.
class IpCheckService : public QObject {
    Q_OBJECT

public:
    explicit IpCheckService(QObject *parent = nullptr);

    /// proxyPort задан — идём через локальный HTTP-прокси sing-box (mixed inbound).
    /// Иначе запрос идёт напрямую.
    void fetch(std::optional<int> proxyPort = std::nullopt);

signals:
    void fetched(std::optional<QString> ip);

private:
    QNetworkAccessManager m_network;
    QString m_endpoint = QStringLiteral("https://api.ipify.org");
};
