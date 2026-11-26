#include "ip_check.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <QTimer>

IpCheckService::IpCheckService(QObject *parent)
    : QObject(parent)
{}

void IpCheckService::fetch(std::optional<int> proxyPort)
{
    if (proxyPort.has_value()) {
        m_network.setProxy(QNetworkProxy(
            QNetworkProxy::HttpProxy,
            QStringLiteral("127.0.0.1"),
            static_cast<quint16>(*proxyPort)));
    } else {
        m_network.setProxy(QNetworkProxy::NoProxy);
    }

    QNetworkRequest req{QUrl(m_endpoint)};

    auto *reply = m_network.get(req);

    // Таймаут вручную (Qt 6.7+ умеет setTransferTimeout)
    QTimer::singleShot(8000, reply, [reply]() {
        if (reply && reply->isRunning()) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fetched(std::nullopt);
            return;
        }
        const QString body = QString::fromUtf8(reply->readAll()).trimmed();
        static const QRegularExpression valid(
            QStringLiteral("^[0-9a-fA-F:.]{3,45}$"));
        if (valid.match(body).hasMatch())
            emit fetched(body);
        else
            emit fetched(std::nullopt);
    });
}
