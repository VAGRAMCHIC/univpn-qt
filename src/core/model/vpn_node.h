#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>

enum class VpnProtocol {
    Vless,
    Trojan,
    Shadowsocks
};

enum class TransportKind {
    Tcp,
    Ws,
    Grpc
};

std::optional<VpnProtocol> protocolFromString(const QString &value);
QString protocolToString(VpnProtocol protocol);

QString transportToString(TransportKind kind);
TransportKind transportFromString(const QString &value);

/// Один узел из подписки.
///
/// tag стабилен между обновлениями подписки: он считается из параметров
/// подключения, поэтому выбранный сервер и результаты url-теста не слетают.
struct VpnNode {
    QString tag;
    VpnProtocol protocol = VpnProtocol::Vless;
    QString name;
    QString server;
    int port = 0;

    QString uuid;
    QString password;
    QString method;
    QString flow;

    TransportKind transport = TransportKind::Tcp;
    QString path;
    QString host;
    QString serviceName;

    bool tls = false;
    QString sni;
    std::optional<QStringList> alpn;   // <-- было QStringList
    QString fingerprint;
    bool allowInsecure = false;
    bool reality = false;
    QString publicKey;
    QString shortId;

    QString endpointLabel() const {
        return QStringLiteral("%1:%2").arg(server).arg(port);
    }

    QJsonObject toJson() const;
    static std::optional<VpnNode> fromJson(const QJsonObject &json);

    /// Outbound sing-box. Поля подставляются только когда релевантны протоколу.
    QJsonObject toOutbound() const;
};
