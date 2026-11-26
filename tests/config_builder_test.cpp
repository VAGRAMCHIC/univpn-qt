#include <QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "core/singbox/config_builder.h"
#include "core/model/vpn_node.h"

namespace {

VpnNode makeVlessNode(const QString &tag, const QString &server, int port)
{
    VpnNode n;
    n.tag = tag;
    n.protocol = VpnProtocol::Vless;
    n.name = tag;
    n.server = server;
    n.port = port;
    n.uuid = QStringLiteral("11111111-2222-3333-4444-555555555555");
    return n;
}

} // namespace

class ConfigBuilderTest : public QObject {
    Q_OBJECT

private slots:
    void emptyNodesSmartTun()
    {
        const QJsonObject cfg = SingBoxConfigBuilder{}
                                    .setSelectedTag(QStringLiteral("proxy"))
                                    .setMode(RoutingMode::Smart)
                                    .setClashApiPort(9999)
                                    .build();

        QCOMPARE(cfg.value(QStringLiteral("log")).toObject()
                     .value(QStringLiteral("level")).toString(),
                 QStringLiteral("warn"));
        QCOMPARE(cfg.value(QStringLiteral("log")).toObject()
                     .value(QStringLiteral("timestamp")).toBool(),
                 true);

        const auto inbounds = cfg.value(QStringLiteral("inbounds")).toArray();
        QCOMPARE(inbounds.size(), 1);
        QCOMPARE(inbounds.at(0).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("tun"));
        QCOMPARE(inbounds.at(0).toObject()
                     .value(QStringLiteral("interface_name")).toString(),
                 QStringLiteral("clgvpn0"));

        const auto experimental = cfg.value(QStringLiteral("experimental")).toObject();
        QCOMPARE(experimental.value(QStringLiteral("clash_api")).toObject()
                     .value(QStringLiteral("external_controller")).toString(),
                 QStringLiteral("127.0.0.1:9999"));
        QCOMPARE(experimental.value(QStringLiteral("clash_api")).toObject()
                     .value(QStringLiteral("secret")).toString(),
                 QString());
        QCOMPARE(experimental.value(QStringLiteral("cache_file")).toObject()
                     .value(QStringLiteral("enabled")).toBool(),
                 true);
    }

    void dnsSmartHasGeositeRule()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Smart)
                             .build();
        const auto dns = cfg.value(QStringLiteral("dns")).toObject();
        const auto rules = dns.value(QStringLiteral("rules")).toArray();
        QCOMPARE(rules.size(), 1);
        QCOMPARE(rules.at(0).toObject().value(QStringLiteral("server")).toString(),
                 QStringLiteral("dns-direct"));
        const auto ruleSet = rules.at(0).toObject()
                                 .value(QStringLiteral("rule_set")).toArray();
        QCOMPARE(ruleSet.size(), 1);
        QCOMPARE(ruleSet.at(0).toString(), QStringLiteral("geosite-ru"));

        QCOMPARE(dns.value(QStringLiteral("final")).toString(),
                 QStringLiteral("dns-remote"));

        const auto servers = dns.value(QStringLiteral("servers")).toArray();
        QCOMPARE(servers.size(), 2);
        QCOMPARE(servers.at(0).toObject().value(QStringLiteral("tag")).toString(),
                 QStringLiteral("dns-direct"));
        QCOMPARE(servers.at(1).toObject().value(QStringLiteral("tag")).toString(),
                 QStringLiteral("dns-remote"));
        QCOMPARE(servers.at(1).toObject().value(QStringLiteral("detour")).toString(),
                 QStringLiteral("proxy"));
    }

    void dnsGlobalHasNoRule()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Global)
                             .build();
        const auto rules = cfg.value(QStringLiteral("dns")).toObject()
                               .value(QStringLiteral("rules")).toArray();
        QVERIFY(rules.isEmpty());
    }

    void mixedInboundInsteadOfTun()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMixedPort(11085)
                             .build();
        const auto inbounds = cfg.value(QStringLiteral("inbounds")).toArray();
        QCOMPARE(inbounds.size(), 1);
        const auto in = inbounds.at(0).toObject();
        QCOMPARE(in.value(QStringLiteral("type")).toString(), QStringLiteral("mixed"));
        QCOMPARE(in.value(QStringLiteral("listen")).toString(),
                 QStringLiteral("127.0.0.1"));
        QCOMPARE(in.value(QStringLiteral("listen_port")).toInt(), 11085);
        QCOMPARE(in.value(QStringLiteral("tag")).toString(), QStringLiteral("mixed-in"));
    }

    void outboundsOrderAndSelector()
    {
        const QList<VpnNode> nodes = {
            makeVlessNode(QStringLiteral("n-a"), QStringLiteral("1.1.1.1"), 443),
            makeVlessNode(QStringLiteral("n-b"), QStringLiteral("2.2.2.2"), 443),
        };

        const auto cfg = SingBoxConfigBuilder{}
                             .setNodes(nodes)
                             .setSelectedTag(QStringLiteral("proxy"))
                             .build();

        const auto outbounds = cfg.value(QStringLiteral("outbounds")).toArray();
        QCOMPARE(outbounds.size(), 2 + 2 + 2); // 2 узла + direct + block + selector + urltest
        QCOMPARE(outbounds.at(0).toObject().value(QStringLiteral("tag")).toString(),
                 QStringLiteral("n-a"));
        QCOMPARE(outbounds.at(1).toObject().value(QStringLiteral("tag")).toString(),
                 QStringLiteral("n-b"));

        // direct
        QCOMPARE(outbounds.at(2).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("direct"));
        // block
        QCOMPARE(outbounds.at(3).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("block"));

        // selector
        const auto selector = outbounds.at(4).toObject();
        QCOMPARE(selector.value(QStringLiteral("type")).toString(),
                 QStringLiteral("selector"));
        QCOMPARE(selector.value(QStringLiteral("tag")).toString(),
                 QStringLiteral("proxy"));
        QCOMPARE(selector.value(QStringLiteral("default")).toString(),
                 QStringLiteral("urltest"));
        const auto selList = selector.value(QStringLiteral("outbounds")).toArray();
        QCOMPARE(selList.size(), 3); // urltest + 2 узла
        QCOMPARE(selList.at(0).toString(), QStringLiteral("urltest"));
        QCOMPARE(selList.at(1).toString(), QStringLiteral("n-a"));
        QCOMPARE(selList.at(2).toString(), QStringLiteral("n-b"));

        // urltest
        const auto urltest = outbounds.at(5).toObject();
        QCOMPARE(urltest.value(QStringLiteral("tag")).toString(),
                 QStringLiteral("urltest"));
        QCOMPARE(urltest.value(QStringLiteral("interval")).toString(),
                 QStringLiteral("3m"));
        QCOMPARE(urltest.value(QStringLiteral("tolerance")).toInt(), 50);
        QCOMPARE(urltest.value(QStringLiteral("outbounds")).toArray().size(), 2);
    }

    void routeSmartHasThreeDirectRules()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Smart)
                             .build();
        const auto route = cfg.value(QStringLiteral("route")).toObject();
        const auto rules = route.value(QStringLiteral("rules")).toArray();
        // sniff + hijack-dns + ip_is_private + geoip-ru + geosite-ru = 5
        QCOMPARE(rules.size(), 5);

        QCOMPARE(rules.at(0).toObject().value(QStringLiteral("action")).toString(),
                 QStringLiteral("sniff"));
        QCOMPARE(rules.at(1).toObject().value(QStringLiteral("action")).toString(),
                 QStringLiteral("hijack-dns"));

        QCOMPARE(rules.at(2).toObject().value(QStringLiteral("ip_is_private")).toBool(),
                 true);
        QCOMPARE(rules.at(2).toObject().value(QStringLiteral("outbound")).toString(),
                 QStringLiteral("direct"));

        QCOMPARE(rules.at(3).toObject().value(QStringLiteral("outbound")).toString(),
                 QStringLiteral("direct"));
        QCOMPARE(rules.at(3).toObject().value(QStringLiteral("rule_set")).toArray()
                     .at(0).toString(),
                 QStringLiteral("geoip-ru"));

        QCOMPARE(rules.at(4).toObject().value(QStringLiteral("rule_set")).toArray()
                     .at(0).toString(),
                 QStringLiteral("geosite-ru"));

        QCOMPARE(route.value(QStringLiteral("final")).toString(),
                 QStringLiteral("proxy"));
        QCOMPARE(route.value(QStringLiteral("default_domain_resolver")).toString(),
                 QStringLiteral("dns-direct"));
        QCOMPARE(route.value(QStringLiteral("auto_detect_interface")).toBool(), true);

        const auto ruleSets = route.value(QStringLiteral("rule_set")).toArray();
        QCOMPARE(ruleSets.size(), 2);
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("remote"));
        QVERIFY(ruleSets.at(0).toObject().value(QStringLiteral("url")).toString()
                    .endsWith(QStringLiteral("/geosite-category-ru.srs")));
    }

    void routeGlobalHasEmptyRuleSet()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Global)
                             .build();
        const auto route = cfg.value(QStringLiteral("route")).toObject();
        QVERIFY(route.value(QStringLiteral("rule_set")).toArray().isEmpty());
        QCOMPARE(route.value(QStringLiteral("final")).toString(),
                 QStringLiteral("proxy"));
        // только sniff + hijack-dns
        QCOMPARE(route.value(QStringLiteral("rules")).toArray().size(), 2);
    }

    void routeWhitelistFinalDirect()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Whitelist)
                             .build();
        const auto route = cfg.value(QStringLiteral("route")).toObject();
        QCOMPARE(route.value(QStringLiteral("final")).toString(),
                 QStringLiteral("direct"));
        // sniff + hijack-dns + ip_is_private = 3
        QCOMPARE(route.value(QStringLiteral("rules")).toArray().size(), 3);
    }

    void ruleSetLocalWhenDirectoryGiven()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Smart)
                             .setRuleSetDirectory(QStringLiteral("/tmp/rulesets"))
                             .build();
        const auto ruleSets = cfg.value(QStringLiteral("route")).toObject()
                                  .value(QStringLiteral("rule_set")).toArray();
        QCOMPARE(ruleSets.size(), 2);
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("local"));
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("path")).toString(),
                 QStringLiteral("/tmp/rulesets/geosite-category-ru.srs"));
        QCOMPARE(ruleSets.at(1).toObject().value(QStringLiteral("path")).toString(),
                 QStringLiteral("/tmp/rulesets/geoip-ru.srs"));
    }

    void ruleSetRemoteByDefault()
    {
        const auto cfg = SingBoxConfigBuilder{}
                             .setMode(RoutingMode::Smart)
                             .build();
        const auto ruleSets = cfg.value(QStringLiteral("route")).toObject()
                                  .value(QStringLiteral("rule_set")).toArray();
        QCOMPARE(ruleSets.size(), 2);
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("type")).toString(),
                 QStringLiteral("remote"));
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("format")).toString(),
                 QStringLiteral("binary"));
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("download_detour")).toString(),
                 QStringLiteral("direct"));
        QCOMPARE(ruleSets.at(0).toObject().value(QStringLiteral("update_interval")).toString(),
                 QStringLiteral("1d"));
    }
};

QTEST_MAIN(ConfigBuilderTest)
#include "config_builder_test.moc"
