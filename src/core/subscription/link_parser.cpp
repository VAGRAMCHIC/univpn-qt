#include "link_parser.h"
#include "../util/base64.h"
#include "../util/stable_hash.h"

#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <stdexcept>

namespace {

[[noreturn]] void fail(const QString &message)
{
    throw std::invalid_argument(message.toStdString());
}

QString tagFor(VpnProtocol protocol, const QString &server, int port,
               const QString &secret, const QStringList &extra)
{
    QStringList parts;
    parts << protocolToString(protocol)
          << server
          << QString::number(port)
          << secret
          << extra;
    return QStringLiteral("n-") + stableHash32(parts.join(QLatin1Char('|')));
}

TransportKind transportFor(const QString &network)
{
    if (network == QLatin1String("ws"))   return TransportKind::Ws;
    if (network == QLatin1String("grpc")) return TransportKind::Grpc;
    return TransportKind::Tcp;
}

std::optional<QStringList> splitAlpn(const QString &value)
{
    if (value.isEmpty()) return std::nullopt;
    QStringList list;
    for (const auto &part : value.split(QLatin1Char(','))) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) list.append(trimmed);
    }
    return list.isEmpty() ? std::nullopt : std::optional<QStringList>(list);
}

QString displayName(const QUrl &url, const QString &fallback)
{
    const QString fragment = QUrl::fromPercentEncoding(url.fragment().toUtf8());
    return fragment.isEmpty() ? fallback : fragment;
}

std::pair<QString, int> endpointFromUrl(const QUrl &url)
{
    const QString host = url.host();
    const int port = url.port(443);
    if (host.isEmpty()) fail(QStringLiteral("В ссылке нет адреса сервера"));
    if (port <= 0 || port > 65535) fail(QStringLiteral("Некорректный порт"));
    return {host, port};
}

VpnNode parseVless(const QUrl &url)
{
    const QString uuid = url.userInfo();
    if (uuid.isEmpty()) fail(QStringLiteral("Во vless-ссылке нет uuid"));

    const auto [server, port] = endpointFromUrl(url);
    const QUrlQuery query(url);
    auto param = [&](const char *key) { return query.queryItemValue(QLatin1String(key)); };

    const QString security = param("security").isEmpty()
                                 ? QStringLiteral("none")
                                 : param("security");
    const QString network = param("type").isEmpty()
                                ? QStringLiteral("tcp")
                                : param("type");
    const QString sni = param("sni");
    const QString host = param("host");
    const QString path = param("path");

    VpnNode n;
    n.tag = tagFor(VpnProtocol::Vless, server, port, uuid,
                   {network, path, sni, param("pbk"), param("sid")});
    n.protocol = VpnProtocol::Vless;
    n.name = displayName(url, QStringLiteral("%1:%2").arg(server).arg(port));
    n.server = server;
    n.port = port;
    n.uuid = uuid;
    n.flow = param("flow");
    n.transport = transportFor(network);
    n.path = path;
    n.host = host;
    n.serviceName = param("serviceName");
    n.tls = (security == QLatin1String("tls") || security == QLatin1String("reality"));
    n.sni = sni;
    n.alpn = splitAlpn(param("alpn"));
    n.fingerprint = param("fp");
    n.allowInsecure = (param("allowInsecure") == QLatin1String("1"));
    n.reality = (security == QLatin1String("reality"));
    n.publicKey = param("pbk");
    n.shortId = param("sid");
    return n;
}

VpnNode parseTrojan(const QUrl &url)
{
    const QString password = url.userInfo();
    if (password.isEmpty()) fail(QStringLiteral("Во trojan-ссылке нет пароля"));

    const auto [server, port] = endpointFromUrl(url);
    const QUrlQuery query(url);
    auto param = [&](const char *key) { return query.queryItemValue(QLatin1String(key)); };

    const QString network = param("type").isEmpty()
                                ? QStringLiteral("tcp")
                                : param("type");

    VpnNode n;
    n.tag = tagFor(VpnProtocol::Trojan, server, port, password,
                   {network, param("path"), param("sni")});
    n.protocol = VpnProtocol::Trojan;
    n.name = displayName(url, QStringLiteral("%1:%2").arg(server).arg(port));
    n.server = server;
    n.port = port;
    n.password = password;
    n.transport = transportFor(network);
    n.path = param("path");
    n.host = param("host");
    n.serviceName = param("serviceName");
    n.tls = true;
    n.sni = param("sni");
    n.alpn = splitAlpn(param("alpn"));
    n.fingerprint = param("fp");
    n.allowInsecure = (param("allowInsecure") == QLatin1String("1"));
    return n;
}

// Uri.parse приводит host к нижнему регистру и ломает base64, поэтому ss разбираем вручную.
VpnNode parseShadowsocks(const QString &link)
{
    const int schemeEnd = link.indexOf(QStringLiteral("://"));
    if (schemeEnd < 0) fail(QStringLiteral("Не удалось разобрать ss-ссылку"));
    QString body = link.mid(schemeEnd + 3);

    QString fragment;
    const int hashAt = body.indexOf(QLatin1Char('#'));
    if (hashAt >= 0) {
        fragment = body.mid(hashAt + 1);
        body = body.left(hashAt);
    }

    QString authority = body;
    const int queryAt = body.indexOf(QLatin1Char('?'));
    if (queryAt >= 0)
        authority = body.left(queryAt);

    QString server;
    int port = 0;
    QString credentials;

    if (authority.contains(QLatin1Char('@'))) {
        const int at = authority.lastIndexOf(QLatin1Char('@'));
        const QString userInfo = authority.left(at);
        const QString hostPort = authority.mid(at + 1);

        const auto decoded = tryDecodeBase64(userInfo);
        credentials = decoded.has_value() ? *decoded : userInfo;

        const int bracket = hostPort.lastIndexOf(QLatin1Char(']'));
        const int colon = hostPort.lastIndexOf(QLatin1Char(':'));
        if (colon <= bracket) fail(QStringLiteral("В ss-ссылке нет порта"));
        server = hostPort.left(colon);
        port = hostPort.mid(colon + 1).toInt();
    } else {
        const auto decoded = tryDecodeBase64(authority);
        if (!decoded.has_value() || !decoded->contains(QLatin1Char('@')))
            fail(QStringLiteral("Не удалось разобрать ss-ссылку"));

        const int at = decoded->lastIndexOf(QLatin1Char('@'));
        credentials = decoded->left(at);
        const QString hostPort = decoded->mid(at + 1);

        const int bracket = hostPort.lastIndexOf(QLatin1Char(']'));
        const int colon = hostPort.lastIndexOf(QLatin1Char(':'));
        if (colon <= bracket) fail(QStringLiteral("В ss-ссылке нет порта"));
        server = hostPort.left(colon);
        port = hostPort.mid(colon + 1).toInt();
    }

    if (port <= 0 || port > 65535) fail(QStringLiteral("Некорректный порт"));

    const int sep = credentials.indexOf(QLatin1Char(':'));
    if (sep <= 0 || sep == credentials.size() - 1)
        fail(QStringLiteral("Ожидается method:password в ss-ссылке"));

    const QString method = credentials.left(sep);
    const QString password = credentials.mid(sep + 1);

    VpnNode n;
    n.tag = tagFor(VpnProtocol::Shadowsocks, server, port, password, {method});
    n.protocol = VpnProtocol::Shadowsocks;
    n.name = fragment.isEmpty()
                 ? QStringLiteral("%1:%2").arg(server).arg(port)
                 : QUrl::fromPercentEncoding(fragment.toUtf8());
    n.server = server;
    n.port = port;
    n.method = method;
    n.password = password;
    return n;
}

} // namespace

std::optional<VpnNode> parseLink(const QString &rawLink)
{
    const QString link = rawLink.trimmed();
    if (link.isEmpty()) return std::nullopt;

    const int schemeEnd = link.indexOf(QStringLiteral("://"));
    if (schemeEnd < 0) return std::nullopt;

    const QString scheme = link.left(schemeEnd).toLower();
    if (scheme == QLatin1String("ss"))
        return parseShadowsocks(link);

    const QUrl url(link, QUrl::StrictMode);
    if (!url.isValid()) fail(QStringLiteral("Некорректная ссылка"));

    if (scheme == QLatin1String("vless"))  return parseVless(url);
    if (scheme == QLatin1String("trojan")) return parseTrojan(url);
    return std::nullopt;
}
