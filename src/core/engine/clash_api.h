#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonObject>
#include <optional>

/// Минимальный клиент Clash API sing-box: версия, трафик, задержка, выбор узла.
class ClashApiClient : public QObject {
    Q_OBJECT
public:
    ClashApiClient(int port,
                   QString secret = {},
                   QString host = QStringLiteral("127.0.0.1"),
                   QObject *parent = nullptr);

    /// Ждёт, пока управляющий API поднимется после старта ядра.
    /// Испускает ready() или readyFailed(QString).
    void waitReady(int timeoutMs = 10000);

    void fetchVersion();
    void fetchTotals();
    void fetchDelay(const QString &tag,
                    const QString &url = QStringLiteral(
                        "https://www.gstatic.com/generate_204"),
                    int timeoutMs = 3000);
    void select(const QString &group, const QString &tag);

signals:
    void ready();
    void readyFailed(const QString &error);
    void versionReceived(const QJsonObject &version);
    void totalsReceived(qint64 download, qint64 upload);
    void delayReceived(const QString &tag, std::optional<int> milliseconds);
    void selectFinished(bool ok, const QString &error);

private:
    QUrl urlFor(const QString &path) const;
    QNetworkRequest makeRequest(const QString &path) const;

    int m_port;
    QString m_secret;
    QString m_host;
    QNetworkAccessManager m_network;
};
