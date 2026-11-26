#include "storage_test.h"

#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTimeZone>

#include "core/settings/app_settings.h"
#include "core/storage/settings_store.h"
#include "core/storage/subscription_store.h"
#include "core/subscription/subscription.h"
#include "core/model/vpn_node.h"

void StorageTest::settingsRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SettingsStore store(dir.filePath(QStringLiteral("settings.json")));

    AppSettings s;
    s.routingMode = RoutingMode::Global;
    s.autoConnect = true;
    s.localeCode = QStringLiteral("ru");
    s.selectedNodeTag = QStringLiteral("n-abc");
    s.killSwitch = true;

    QVERIFY(store.save(s));
    const AppSettings loaded = store.load();

    QCOMPARE(loaded.routingMode, RoutingMode::Global);
    QCOMPARE(loaded.autoConnect, true);
    QVERIFY(loaded.localeCode.has_value());
    QCOMPARE(*loaded.localeCode, QStringLiteral("ru"));
    QVERIFY(loaded.selectedNodeTag.has_value());
    QCOMPARE(*loaded.selectedNodeTag, QStringLiteral("n-abc"));
    QCOMPARE(loaded.killSwitch, true);
}

void StorageTest::settingsOmitsOptionalFields()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SettingsStore store(dir.filePath(QStringLiteral("settings.json")));

    AppSettings s;
    s.routingMode = RoutingMode::Smart;
    QVERIFY(store.save(s));

    QFile f(store.filePath());
    QVERIFY(f.open(QIODevice::ReadOnly));
    const auto doc = QJsonDocument::fromJson(f.readAll());
    const auto obj = doc.object();
    QVERIFY(!obj.contains(QStringLiteral("localeCode")));
    QVERIFY(!obj.contains(QStringLiteral("selectedNodeTag")));
    QCOMPARE(obj.value(QStringLiteral("routingMode")).toString(),
             QStringLiteral("smart"));
    QCOMPARE(obj.value(QStringLiteral("autoConnect")).toBool(), false);
    QCOMPARE(obj.value(QStringLiteral("killSwitch")).toBool(), false);
}

void StorageTest::settingsMissingFileReturnsDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SettingsStore store(dir.filePath(QStringLiteral("nope.json")));

    const AppSettings s = store.load();
    QCOMPARE(s.routingMode, RoutingMode::Smart);
    QCOMPARE(s.autoConnect, false);
    QVERIFY(!s.localeCode.has_value());
    QVERIFY(!s.selectedNodeTag.has_value());
    QCOMPARE(s.killSwitch, false);
}

void StorageTest::settingsCorruptJsonReturnsDefaults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ broken json !!!");
    }
    SettingsStore store(path);
    const AppSettings s = store.load();
    QCOMPARE(s.routingMode, RoutingMode::Smart);
}

void StorageTest::subscriptionRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SubscriptionStore store(dir.filePath(QStringLiteral("subs.json")));

    VpnNode node;
    node.tag = QStringLiteral("n-1");
    node.protocol = VpnProtocol::Vless;
    node.name = QStringLiteral("Test");
    node.server = QStringLiteral("1.2.3.4");
    node.port = 443;
    node.uuid = QStringLiteral("uuid-1");
    node.tls = true;
    node.sni = QStringLiteral("example.com");
    node.alpn = QStringList{QStringLiteral("h2")};

    SubscriptionTraffic traffic;
    traffic.upload = 1000;
    traffic.download = 2000;
    traffic.total = 100000;
    traffic.expire = QDateTime::fromSecsSinceEpoch(1767225600, QTimeZone::UTC);

    Subscription sub;
    sub.id = QStringLiteral("s-abc");
    sub.url = QStringLiteral("https://example.com/sub");
    sub.title = QStringLiteral("My VPN");
    sub.nodes = {node};
    sub.updatedAt = QDateTime::fromSecsSinceEpoch(1700000000, QTimeZone::UTC);
    sub.traffic = traffic;
    sub.updateIntervalMinutes = 360;

    QVERIFY(store.save({sub}));
    const auto loaded = store.load();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0].id, sub.id);
    QCOMPARE(loaded[0].url, sub.url);
    QCOMPARE(loaded[0].title, sub.title);
    QCOMPARE(loaded[0].nodes.size(), 1);
    QCOMPARE(loaded[0].nodes[0].tag, node.tag);
    QVERIFY(loaded[0].nodes[0].alpn.has_value());
    QCOMPARE(*loaded[0].nodes[0].alpn, *node.alpn);
    QVERIFY(loaded[0].traffic.has_value());
    QCOMPARE(loaded[0].traffic->used(), 3000LL);
    QCOMPARE(loaded[0].updateIntervalMinutes, 360);
    QCOMPARE(loaded[0].updatedAt.toSecsSinceEpoch(), sub.updatedAt.toSecsSinceEpoch());
}

void StorageTest::subscriptionReadsFlutterFormat()
{
    const QByteArray json = R"({
  "version": 1,
  "subscriptions": [
    {
      "id": "s-0a1b2c3d",
      "url": "https://example.com/sub",
      "title": "Panel",
      "nodes": [
        {
          "tag": "n-deadbeef",
          "protocol": "vless",
          "name": "Node 1",
          "server": "5.6.7.8",
          "port": 443,
          "uuid": "11111111-1111-1111-1111-111111111111",
          "transport": "ws",
          "path": "/ws",
          "host": "cdn.example.com",
          "tls": true,
          "sni": "cdn.example.com",
          "alpn": ["h2"],
          "fingerprint": "chrome",
          "allowInsecure": false,
          "reality": false
        }
      ],
      "updatedAt": "2024-11-14T22:13:20Z",
      "traffic": {
        "upload": 123,
        "download": 456,
        "total": 1000,
        "expire": "2026-01-01T00:00:00Z"
      },
      "updateIntervalMinutes": 720
    }
  ]
})";

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("subs.json"));
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
    }

    SubscriptionStore store(path);
    const auto loaded = store.load();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0].id, QStringLiteral("s-0a1b2c3d"));
    QCOMPARE(loaded[0].nodes.size(), 1);

    const auto &n = loaded[0].nodes[0];
    QCOMPARE(n.tag, QStringLiteral("n-deadbeef"));
    QCOMPARE(n.protocol, VpnProtocol::Vless);
    QCOMPARE(n.transport, TransportKind::Ws);
    QCOMPARE(n.path, QStringLiteral("/ws"));
    QCOMPARE(n.host, QStringLiteral("cdn.example.com"));
    QCOMPARE(n.tls, true);
    QVERIFY(n.alpn.has_value());
    QCOMPARE(*n.alpn, QStringList{QStringLiteral("h2")});

    QVERIFY(loaded[0].traffic.has_value());
    QCOMPARE(loaded[0].traffic->upload, 123LL);
    QCOMPARE(loaded[0].traffic->download, 456LL);
    QCOMPARE(loaded[0].traffic->total, 1000LL);
    QVERIFY(loaded[0].traffic->expire.has_value());
}

void StorageTest::subscriptionIdStable()
{
    const QString a = subscriptionIdFor(QStringLiteral("https://example.com/sub"));
    const QString b = subscriptionIdFor(QStringLiteral("https://example.com/sub"));
    QCOMPARE(a, b);
    QVERIFY(a.startsWith(QStringLiteral("s-")));
    QCOMPARE(a.size(), 2 + 8);

    const QString c = subscriptionIdFor(QStringLiteral("https://other.com/sub"));
    QVERIFY(a != c);
}

QTEST_MAIN(StorageTest)
