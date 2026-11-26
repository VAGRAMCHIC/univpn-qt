#include "clash_api.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QThread>
#include <QDebug>

ClashApiClient::ClashApiClient(int port, QString secret, QString host,
                               QObject *parent)
    : QObject(parent)
    , m_port(port)
    , m_secret(std::move(secret))
    , m_host(host.isEmpty() ? QStringLiteral("127.0.0.1") : std::move(host))
{}

QUrl ClashApiClient::urlFor(const QString &path) const
{
    QUrl u;
    u.setScheme(QStringLiteral("http"));
    u.setHost(m_host);
    u.setPort(m_port);
    u.setPath(path);
    return u;
}

QNetworkRequest ClashApiClient::makeRequest(const QString &path) const
{
    QNetworkRequest req(urlFor(path));
    if (!m_secret.isEmpty())
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + m_secret.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    return req;
}

void ClashApiClient::waitReady(int timeoutMs)
{
    qDebug() << "[clash] waitReady start, timeout:" << timeoutMs;
    QElapsedTimer elapsed;
    elapsed.start();

    while (elapsed.elapsed() < timeoutMs) {
        auto *reply = m_network.get(makeRequest(QStringLiteral("/version")));
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(500);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();
        loop.exec();

        const bool ok = reply->error() == QNetworkReply::NoError;
        const QByteArray body = reply->readAll();
        qDebug() << "[clash] poll: ok=" << ok << "error=" << reply->error()
                 << "body=" << body;
        reply->deleteLater();

        if (ok && !body.isEmpty()) {
            qDebug() << "[clash] emitting ready()";
            emit ready();
            return;
        }
        QThread::msleep(150);
    }

    qDebug() << "[clash] timeout, emitting readyFailed()";
    emit readyFailed(QStringLiteral("Clash API не поднялся за %1 мс")
                         .arg(timeoutMs));
}

void ClashApiClient::fetchVersion()
{
    auto *reply = m_network.get(makeRequest(QStringLiteral("/version")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) emit versionReceived(doc.object());
    });
}

void ClashApiClient::fetchTotals()
{
    auto *reply = m_network.get(makeRequest(QStringLiteral("/connections")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;
        const auto o = doc.object();
        const qint64 down =
            o.value(QStringLiteral("downloadTotal")).toVariant().toLongLong();
        const qint64 up =
            o.value(QStringLiteral("uploadTotal")).toVariant().toLongLong();
        emit totalsReceived(down, up);
    });
}

void ClashApiClient::fetchDelay(const QString &tag, const QString &url,
                                int timeoutMs)
{
    QUrl u = urlFor(QStringLiteral("/proxies/%1/delay")
                        .arg(QString::fromUtf8(QUrl::toPercentEncoding(tag))));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("url"), url);
    q.addQueryItem(QStringLiteral("timeout"), QString::number(timeoutMs));
    u.setQuery(q);

    QNetworkRequest req(u);
    if (!m_secret.isEmpty())
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + m_secret.toUtf8());

    auto *reply = m_network.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tag]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit delayReceived(tag, std::nullopt);
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit delayReceived(tag, std::nullopt);
            return;
        }
        const auto v = doc.object().value(QStringLiteral("delay"));
        emit delayReceived(tag, v.isDouble() ? std::optional<int>(v.toInt())
                                             : std::nullopt);
    });
}

void ClashApiClient::select(const QString &group, const QString &tag)
{
    auto req = makeRequest(
        QStringLiteral("/proxies/%1")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(group))));
    QJsonObject body;
    body[QStringLiteral("name")] = tag;

    auto *reply = m_network.put(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const bool ok = reply->error() == QNetworkReply::NoError;
        emit selectFinished(ok, ok ? QString() : reply->errorString());
    });
}
