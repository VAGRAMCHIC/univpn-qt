#include "vpn_node.h"

std::optional<VpnProtocol> protocolFromString(const QString &value)
{
    if (value == QLatin1String("vless")) return VpnProtocol::Vless;
    if (value == QLatin1String("trojan")) return VpnProtocol::Trojan;
    if (value == QLatin1String("ss") || value == QLatin1String("shadowsocks"))
        return VpnProtocol::Shadowsocks;
    return std::nullopt;
}

QString protocolToString(VpnProtocol protocol)
{
    switch (protocol) {
    case VpnProtocol::Vless:       return QStringLiteral("vless");
    case VpnProtocol::Trojan:      return QStringLiteral("trojan");
    case VpnProtocol::Shadowsocks: return QStringLiteral("shadowsocks");
    }
    return {};
}

QString transportToString(TransportKind kind)
{
    switch (kind) {
    case TransportKind::Tcp:  return QStringLiteral("tcp");
    case TransportKind::Ws:   return QStringLiteral("ws");
    case TransportKind::Grpc: return QStringLiteral("grpc");
    }
    return {};
}

TransportKind transportFromString(const QString &value)
{
    if (value == QLatin1String("ws"))   return TransportKind::Ws;
    if (value == QLatin1String("grpc")) return TransportKind::Grpc;
    return TransportKind::Tcp;
}

QJsonObject VpnNode::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("tag")]        = tag;
    o[QStringLiteral("protocol")]   = protocolToString(protocol);
    o[QStringLiteral("name")]       = name;
    o[QStringLiteral("server")]     = server;
    o[QStringLiteral("port")]       = port;

    if (!uuid.isEmpty())        o[QStringLiteral("uuid")]        = uuid;
    if (!password.isEmpty())    o[QStringLiteral("password")]    = password;
    if (!method.isEmpty())      o[QStringLiteral("method")]      = method;
    if (!flow.isEmpty())        o[QStringLiteral("flow")]        = flow;

    o[QStringLiteral("transport")] = transportToString(transport);

    if (!path.isEmpty())        o[QStringLiteral("path")]        = path;
    if (!host.isEmpty())        o[QStringLiteral("host")]        = host;
    if (!serviceName.isEmpty()) o[QStringLiteral("serviceName")] = serviceName;

    o[QStringLiteral("tls")] = tls;

    if (!sni.isEmpty())         o[QStringLiteral("sni")]         = sni;
    if (alpn.has_value())
    o[QStringLiteral("alpn")] = QJsonArray::fromStringList(*alpn);

    if (!fingerprint.isEmpty()) o[QStringLiteral("fingerprint")] = fingerprint;

    o[QStringLiteral("allowInsecure")] = allowInsecure;
    o[QStringLiteral("reality")]       = reality;

    if (!publicKey.isEmpty())   o[QStringLiteral("publicKey")]   = publicKey;
    if (!shortId.isEmpty())     o[QStringLiteral("shortId")]     = shortId;

    return o;
}

std::optional<VpnNode> VpnNode::fromJson(const QJsonObject &json)
{
    const auto proto = protocolFromString(json.value(QStringLiteral("protocol")).toString());
    if (!proto.has_value()) return std::nullopt;

    VpnNode node;
    node.tag       = json.value(QStringLiteral("tag")).toString();
    node.protocol  = *proto;
    node.name      = json.value(QStringLiteral("name")).toString();
    node.server    = json.value(QStringLiteral("server")).toString();
    node.port      = json.value(QStringLiteral("port")).toInt();

    if (node.tag.isEmpty() || node.server.isEmpty() || node.port <= 0)
        return std::nullopt;

    node.uuid        = json.value(QStringLiteral("uuid")).toString();
    node.password    = json.value(QStringLiteral("password")).toString();
    node.method      = json.value(QStringLiteral("method")).toString();
    node.flow        = json.value(QStringLiteral("flow")).toString();

    node.transport   = transportFromString(
        json.value(QStringLiteral("transport")).toString(QStringLiteral("tcp")));

    node.path        = json.value(QStringLiteral("path")).toString();
    node.host        = json.value(QStringLiteral("host")).toString();
    node.serviceName = json.value(QStringLiteral("serviceName")).toString();

    node.tls         = json.value(QStringLiteral("tls")).toBool(false);

    node.sni         = json.value(QStringLiteral("sni")).toString();

    if (json.contains(QStringLiteral("alpn")) && json.value(QStringLiteral("alpn")).isArray()) {
        QStringList list;
        for (const auto &v : json.value(QStringLiteral("alpn")).toArray())
            list.append(v.toString());
        node.alpn = list;
    }

    node.fingerprint = json.value(QStringLiteral("fingerprint")).toString();

    node.allowInsecure = json.value(QStringLiteral("allowInsecure")).toBool(false);
    node.reality       = json.value(QStringLiteral("reality")).toBool(false);

    node.publicKey   = json.value(QStringLiteral("publicKey")).toString();
    node.shortId     = json.value(QStringLiteral("shortId")).toString();

    return node;
}

QJsonObject VpnNode::toOutbound() const
{
    QJsonObject outbound;
    outbound[QStringLiteral("type")]        = protocolToString(protocol);
    outbound[QStringLiteral("tag")]         = tag;
    outbound[QStringLiteral("server")]      = server;
    outbound[QStringLiteral("server_port")] = port;

    switch (protocol) {
    case VpnProtocol::Vless:
        outbound[QStringLiteral("uuid")] = uuid;
        outbound[QStringLiteral("packet_encoding")] = QStringLiteral("xudp");
        if (!flow.isEmpty())
            outbound[QStringLiteral("flow")] = flow;
        break;
    case VpnProtocol::Trojan:
        outbound[QStringLiteral("password")] = password;
        break;
    case VpnProtocol::Shadowsocks:
        outbound[QStringLiteral("method")]   = method;
        outbound[QStringLiteral("password")] = password;
        break;
    }

    if (tls) {
        QJsonObject tlsBlock;
        tlsBlock[QStringLiteral("enabled")] = true;

        QString serverName = sni;
        if (serverName.isEmpty()) serverName = host;
        if (serverName.isEmpty()) serverName = server;
        tlsBlock[QStringLiteral("server_name")] = serverName;

        // Reality без uTLS ядро не принимает, поэтому подставляем chrome.
        QString effectiveFp = fingerprint;
        if (effectiveFp.isEmpty() && reality)
            effectiveFp = QStringLiteral("chrome");
        if (!effectiveFp.isEmpty()) {
            QJsonObject utls;
            utls[QStringLiteral("enabled")]     = true;
            utls[QStringLiteral("fingerprint")] = effectiveFp;
            tlsBlock[QStringLiteral("utls")] = utls;
        }
        
	if (alpn.has_value() && !alpn->isEmpty())
            tlsBlock[QStringLiteral("alpn")] = QJsonArray::fromStringList(*alpn);

        if (allowInsecure)
            tlsBlock[QStringLiteral("insecure")] = true;

        if (reality) {
            QJsonObject r;
            r[QStringLiteral("enabled")]    = true;
            r[QStringLiteral("public_key")] = publicKey;
            if (!shortId.isEmpty())
                r[QStringLiteral("short_id")] = shortId;
            tlsBlock[QStringLiteral("reality")] = r;
        }

        outbound[QStringLiteral("tls")] = tlsBlock;
    }

    switch (transport) {
    case TransportKind::Tcp:
        break;
    case TransportKind::Ws: {
        QJsonObject tr;
        tr[QStringLiteral("type")] = QStringLiteral("ws");
        tr[QStringLiteral("path")] = path.isEmpty() ? QStringLiteral("/") : path;
        if (!host.isEmpty()) {
            QJsonObject headers;
            headers[QStringLiteral("Host")] = host;
            tr[QStringLiteral("headers")]   = headers;
        }
        outbound[QStringLiteral("transport")] = tr;
        break;
    }
    case TransportKind::Grpc: {
        QJsonObject tr;
        tr[QStringLiteral("type")]         = QStringLiteral("grpc");
        tr[QStringLiteral("service_name")] = serviceName; // пусто -> ""
        outbound[QStringLiteral("transport")] = tr;
        break;
    }
    }

    return outbound;
}
