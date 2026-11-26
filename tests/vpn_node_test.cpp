#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/model/vpn_node.h"

class VpnNodeTest : public QObject {
    Q_OBJECT

private slots:
    void protocolParse()
    {
        QCOMPARE(protocolFromString(QStringLiteral("vless")).value(), VpnProtocol::Vless);
        QCOMPARE(protocolFromString(QStringLiteral("ss")).value(), VpnProtocol::Shadowsocks);
        QCOMPARE(protocolFromString(QStringLiteral("shadowsocks")).value(), VpnProtocol::Shadowsocks);
        QVERIFY(!protocolFromString(QStringLiteral("wireguard")).has_value());
    }

    void vlessOutboundHasNoPassword()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-1");
        n.protocol = VpnProtocol::Vless;
        n.server = QStringLiteral("1.2.3.4");
        n.port = 443;
        n.uuid = QStringLiteral("uuid-1");

        const QJsonObject o = n.toOutbound();
        QCOMPARE(o.value(QStringLiteral("type")).toString(), QStringLiteral("vless"));
        QCOMPARE(o.value(QStringLiteral("uuid")).toString(), QStringLiteral("uuid-1"));
        QCOMPARE(o.value(QStringLiteral("packet_encoding")).toString(), QStringLiteral("xudp"));
        QVERIFY(!o.contains(QStringLiteral("password")));  // не должно протечь из trojan-case
        QVERIFY(!o.contains(QStringLiteral("flow")));      // пустой flow не пишется
    }

    void trojanOutboundHasPassword()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-2");
        n.protocol = VpnProtocol::Trojan;
        n.server = QStringLiteral("2.2.2.2");
        n.port = 443;
        n.password = QStringLiteral("secret");

        const QJsonObject o = n.toOutbound();
        QCOMPARE(o.value(QStringLiteral("password")).toString(), QStringLiteral("secret"));
    }

    void shadowsocksOutbound()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-3");
        n.protocol = VpnProtocol::Shadowsocks;
        n.server = QStringLiteral("3.3.3.3");
        n.port = 8388;
        n.method = QStringLiteral("aes-256-gcm");
        n.password = QStringLiteral("pw");

        const QJsonObject o = n.toOutbound();
        QCOMPARE(o.value(QStringLiteral("method")).toString(), QStringLiteral("aes-256-gcm"));
        QCOMPARE(o.value(QStringLiteral("password")).toString(), QStringLiteral("pw"));
    }

    void tlsServerNameFallback()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-4");
        n.protocol = VpnProtocol::Vless;
        n.server = QStringLiteral("4.4.4.4");
        n.port = 443;
        n.uuid = QStringLiteral("u");
        n.tls = true;
        n.host = QStringLiteral("cdn.example.com");

        auto o = n.toOutbound();
        auto tls = o.value(QStringLiteral("tls")).toObject();
        QCOMPARE(tls.value(QStringLiteral("server_name")).toString(),
                 QStringLiteral("cdn.example.com"));

        // если host пуст — fallback на server
        n.host.clear();
        o = n.toOutbound();
        tls = o.value(QStringLiteral("tls")).toObject();
        QCOMPARE(tls.value(QStringLiteral("server_name")).toString(),
                 QStringLiteral("4.4.4.4"));
    }

    void realityImpliesChromeFingerprint()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-5");
        n.protocol = VpnProtocol::Vless;
        n.server = QStringLiteral("5.5.5.5");
        n.port = 443;
        n.uuid = QStringLiteral("u");
        n.tls = true;
        n.reality = true;
        n.publicKey = QStringLiteral("pub");
        n.shortId = QStringLiteral("sid");

        const auto o = n.toOutbound();
        const auto tls = o.value(QStringLiteral("tls")).toObject();
        const auto utls = tls.value(QStringLiteral("utls")).toObject();
        QCOMPARE(utls.value(QStringLiteral("fingerprint")).toString(),
                 QStringLiteral("chrome"));

        const auto r = tls.value(QStringLiteral("reality")).toObject();
        QCOMPARE(r.value(QStringLiteral("public_key")).toString(), QStringLiteral("pub"));
        QCOMPARE(r.value(QStringLiteral("short_id")).toString(), QStringLiteral("sid"));
    }

    void wsTransport()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-6");
        n.protocol = VpnProtocol::Vless;
        n.server = QStringLiteral("6.6.6.6");
        n.port = 443;
        n.uuid = QStringLiteral("u");
        n.transport = TransportKind::Ws;
        n.path = QStringLiteral("/ws");
        n.host = QStringLiteral("cdn.example.com");

        const auto tr = n.toOutbound()
                            .value(QStringLiteral("transport")).toObject();
        QCOMPARE(tr.value(QStringLiteral("type")).toString(), QStringLiteral("ws"));
        QCOMPARE(tr.value(QStringLiteral("path")).toString(), QStringLiteral("/ws"));
        QCOMPARE(tr.value(QStringLiteral("headers")).toObject()
                     .value(QStringLiteral("Host")).toString(),
                 QStringLiteral("cdn.example.com"));
    }

    void jsonRoundtrip()
    {
        VpnNode n;
        n.tag = QStringLiteral("n-7");
        n.protocol = VpnProtocol::Vless;
        n.name = QStringLiteral("Test");
        n.server = QStringLiteral("7.7.7.7");
        n.port = 443;
        n.uuid = QStringLiteral("u-7");
        n.transport = TransportKind::Grpc;
        n.serviceName = QStringLiteral("svc");
        n.tls = true;
        n.sni = QStringLiteral("sni.example.com");
        n.alpn = QStringList{QStringLiteral("h2"), QStringLiteral("http/1.1")};
        n.reality = true;
        n.publicKey = QStringLiteral("pk");
        n.shortId = QStringLiteral("sid");

        const auto restored = VpnNode::fromJson(n.toJson());
        QVERIFY(restored.has_value());
        QCOMPARE(restored->tag, n.tag);
        QCOMPARE(restored->protocol, n.protocol);
        QCOMPARE(restored->transport, n.transport);
        QCOMPARE(restored->sni, n.sni);
        QVERIFY(restored->alpn.has_value());
        QCOMPARE(*restored->alpn, *n.alpn);
        QCOMPARE(restored->publicKey, n.publicKey);
    }
};

QTEST_MAIN(VpnNodeTest)
#include "vpn_node_test.moc"
