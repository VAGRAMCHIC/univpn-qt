#include "subscription_service.h"
#include "subscription_parser.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QRegularExpression>

SubscriptionService::SubscriptionService(QObject *parent)
    : QObject(parent) {}

void SubscriptionService::fetch(const QString &rawUrl)
{
    
    const QUrl url(rawUrl.trimmed());
    const QString scheme = url.scheme();
    if (!url.isValid()
        || (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
        emit failed(QStringLiteral("Нужна ссылка http:// или https://"));
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("clgvpn/0.1.0"));
    request.setRawHeader("Accept", "*/*");
    request.setTransferTimeout(20000);

    QNetworkReply *reply = m_network.get(request);


    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit failed(QStringLiteral("Не удалось подключиться: %1")
                            .arg(reply->errorString()));
            return;
        }

        const QByteArray body = reply->readAll();
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            emit failed(QStringLiteral("Сервер ответил %1").arg(status));
            return;
        }

        QMap<QString, QString> headers;
        for (const auto &pair : reply->rawHeaderPairs())
            headers.insert(QString::fromLatin1(pair.first),
                           QString::fromLatin1(pair.second));

        QList<VpnNode> nodes;
        try {
            nodes = parseSubscriptionBody(QString::fromUtf8(body));
        } catch (const std::invalid_argument &e) {
            emit failed(QString::fromStdString(e.what()));
            return;
        }

        if (nodes.isEmpty()) {
            emit failed(QStringLiteral("В подписке не нашлось поддерживаемых узлов"));
            return;
        }

        const auto meta = SubscriptionMeta::parse(headers);

        Subscription sub;
        sub.id = subscriptionIdFor(url.toString());
        sub.url = url.toString();
        sub.title = meta.title.value_or(url.host());
        sub.nodes = nodes;
        sub.updatedAt = QDateTime::currentDateTimeUtc();
        sub.traffic = meta.traffic;
        sub.updateIntervalMinutes = meta.updateIntervalHours.value_or(12) * 60;

        emit finished(sub);
    });
}
