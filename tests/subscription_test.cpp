#include "subscription_test.h"

#include <QtTest>
#include <stdexcept>

#include "core/subscription/link_parser.h"
#include "core/subscription/subscription_parser.h"
#include "core/subscription/auto_refresh.h"
#include "core/subscription/subscription.h"

void SubscriptionTest::vlessReality()
{
    const auto node = parseLink(QStringLiteral(
        "vless://11111111-2222-3333-4444-555555555555@1.2.3.4:443"
        "?security=reality&type=grpc&serviceName=svc&sni=cdn.example.com"
        "&fp=chrome&pbk=abc&sid=def&flow=xtls-rprx-vision#My%20Node"));
    QVERIFY(node.has_value());
    QCOMPARE(node->protocol, VpnProtocol::Vless);
    QCOMPARE(node->server, QStringLiteral("1.2.3.4"));
    QCOMPARE(node->port, 443);
    QCOMPARE(node->uuid, QStringLiteral("11111111-2222-3333-4444-555555555555"));
    QCOMPARE(node->transport, TransportKind::Grpc);
    QCOMPARE(node->serviceName, QStringLiteral("svc"));
    QCOMPARE(node->sni, QStringLiteral("cdn.example.com"));
    QCOMPARE(node->fingerprint, QStringLiteral("chrome"));
    QCOMPARE(node->publicKey, QStringLiteral("abc"));
    QCOMPARE(node->shortId, QStringLiteral("def"));
    QCOMPARE(node->flow, QStringLiteral("xtls-rprx-vision"));
    QCOMPARE(node->reality, true);
    QCOMPARE(node->name, QStringLiteral("My Node"));
    QVERIFY(node->tag.startsWith(QStringLiteral("n-")));
}

void SubscriptionTest::vlessInvalidUuid()
{
    QVERIFY_EXCEPTION_THROWN(
        parseLink(QStringLiteral("vless://@1.2.3.4:443")),
        std::invalid_argument);
}

void SubscriptionTest::trojanPassword()
{
    const auto node = parseLink(QStringLiteral(
        "trojan://secret@2.2.2.2:8443?type=ws&path=/ws&host=cdn.example.com#T"));
    QVERIFY(node.has_value());
    QCOMPARE(node->protocol, VpnProtocol::Trojan);
    QCOMPARE(node->password, QStringLiteral("secret"));
    QCOMPARE(node->transport, TransportKind::Ws);
    QCOMPARE(node->path, QStringLiteral("/ws"));
    QCOMPARE(node->host, QStringLiteral("cdn.example.com"));
    QCOMPARE(node->tls, true);
}

void SubscriptionTest::shadowsocksBase64()
{
    // base64("aes-256-gcm:pw") = YWVzLTI1Ni1nY206cHc=
    // Полная authority = base64("aes-256-gcm:pw@3.3.3.3:8388")
    const QString encoded = QString::fromLatin1(
        QByteArray("aes-256-gcm:pw@3.3.3.3:8388").toBase64());
    const auto node = parseLink(QStringLiteral("ss://%1#SS").arg(encoded));
    QVERIFY(node.has_value());
    QCOMPARE(node->protocol, VpnProtocol::Shadowsocks);
    QCOMPARE(node->server, QStringLiteral("3.3.3.3"));
    QCOMPARE(node->port, 8388);
    QCOMPARE(node->method, QStringLiteral("aes-256-gcm"));
    QCOMPARE(node->password, QStringLiteral("pw"));
    QCOMPARE(node->name, QStringLiteral("SS"));
}

void SubscriptionTest::shadowsocksUserAtHost()
{
    // userinfo в base64, host:port — открытым текстом
    const QString userInfo = QString::fromLatin1(
        QByteArray("aes-256-gcm:pw").toBase64());
    const auto node = parseLink(QStringLiteral("ss://%1@4.4.4.4:8388#X").arg(userInfo));
    QVERIFY(node.has_value());
    QCOMPARE(node->server, QStringLiteral("4.4.4.4"));
    QCOMPARE(node->port, 8388);
    QCOMPARE(node->method, QStringLiteral("aes-256-gcm"));
}

void SubscriptionTest::shadowsocksInvalid()
{
    QVERIFY_EXCEPTION_THROWN(
        parseLink(QStringLiteral("ss://not-base64")),
        std::invalid_argument);
}

void SubscriptionTest::bodyFromPlainList()
{
    const QString body = QStringLiteral(
        "vless://11111111-2222-3333-4444-555555555555@1.2.3.4:443#A\n"
        "trojan://pw@2.2.2.2:443#B\n"
        "garbage line\n");
    const auto nodes = parseSubscriptionBody(body);
    QCOMPARE(nodes.size(), 2);
    QCOMPARE(nodes[0].name, QStringLiteral("A"));
    QCOMPARE(nodes[1].name, QStringLiteral("B"));
}

void SubscriptionTest::bodyFromBase64()
{
    const QString plain = QStringLiteral(
        "vless://11111111-2222-3333-4444-555555555555@1.2.3.4:443#A\n"
        "trojan://pw@2.2.2.2:443#B");
    const QString encoded = QString::fromLatin1(plain.toUtf8().toBase64());
    const auto nodes = parseSubscriptionBody(encoded);
    QCOMPARE(nodes.size(), 2);
}

void SubscriptionTest::bodyRejectsJson()
{
    QVERIFY_EXCEPTION_THROWN(
        parseSubscriptionBody(QStringLiteral("{\"outbounds\":[]}")),
        std::invalid_argument);
}

void SubscriptionTest::bodySkipsBrokenLines()
{
    const QString body = QStringLiteral(
        "vless://@1.2.3.4:443\n"                          // битый
        "trojan://pw@2.2.2.2:443#Good\n");
    const auto nodes = parseSubscriptionBody(body);
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes[0].name, QStringLiteral("Good"));
}

void SubscriptionTest::metaFromHeaders()
{
    QMap<QString, QString> headers;
    headers[QStringLiteral("profile-title")] = QStringLiteral("My Panel");
    headers[QStringLiteral("subscription-userinfo")] =
        QStringLiteral("upload=100; download=200; total=1000; expire=1767225600");
    headers[QStringLiteral("profile-update-interval")] = QStringLiteral("6");

    const auto meta = SubscriptionMeta::parse(headers);
    QVERIFY(meta.title.has_value());
    QCOMPARE(*meta.title, QStringLiteral("My Panel"));
    QVERIFY(meta.traffic.has_value());
    QCOMPARE(meta.traffic->upload, 100LL);
    QCOMPARE(meta.traffic->download, 200LL);
    QCOMPARE(meta.traffic->total, 1000LL);
    QVERIFY(meta.traffic->expire.has_value());
    QVERIFY(meta.updateIntervalHours.has_value());
    QCOMPARE(*meta.updateIntervalHours, 6);
}

void SubscriptionTest::dueForRefresh()
{
    Subscription sub;
    sub.updatedAt = QDateTime::currentDateTimeUtc().addSecs(-13 * 3600);
    sub.updateIntervalMinutes = 12 * 60;
    QVERIFY(isDueForRefresh(sub, QDateTime::currentDateTimeUtc()));

    sub.updatedAt = QDateTime::currentDateTimeUtc().addSecs(-1 * 3600);
    QVERIFY(!isDueForRefresh(sub, QDateTime::currentDateTimeUtc()));
}

QTEST_MAIN(SubscriptionTest)

