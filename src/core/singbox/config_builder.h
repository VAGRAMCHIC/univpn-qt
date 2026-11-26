#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "../model/vpn_node.h"

/// Режим маршрутизации.
enum class RoutingMode {
    Smart,      // RU и приватные сети — напрямую, остальное — через VPN
    Global,     // весь трафик через VPN
    Whitelist   // всё напрямую, через VPN — только явно перечисленные
};

QString routingModeToString(RoutingMode mode);
RoutingMode routingModeFromString(const QString &value);

/// Собирает JSON-конфиг sing-box из узлов и настроек.
///
/// Один и тот же конфиг уходит и в libbox на Android, и в сайдкар на десктопе.
class SingBoxConfigBuilder {
public:
    static constexpr const char *defaultRuleSetBaseUrl =
        "https://github.com/clgvpn/rulesets/releases/latest/download";

    SingBoxConfigBuilder() = default;

    SingBoxConfigBuilder &setNodes(QList<VpnNode> nodes);
    SingBoxConfigBuilder &setSelectedTag(QString tag);
    SingBoxConfigBuilder &setMode(RoutingMode mode);
    SingBoxConfigBuilder &setClashApiPort(int port);
    SingBoxConfigBuilder &setClashApiSecret(QString secret);
    SingBoxConfigBuilder &setTunInterfaceName(QString name);
    SingBoxConfigBuilder &setTunAddress(QString address);
    SingBoxConfigBuilder &setTunStack(QString stack);
    SingBoxConfigBuilder &setRuleSetBaseUrl(QString url);
    SingBoxConfigBuilder &setLogLevel(QString level);
    SingBoxConfigBuilder &setMixedPort(std::optional<int> port);
    SingBoxConfigBuilder &setRuleSetDirectory(std::optional<QString> dir);

    QJsonObject build() const;

private:
    QJsonObject buildDns() const;
    QJsonObject buildTunInbound() const;
    QJsonObject buildMixedInbound(int port) const;
    QJsonObject buildSelector(const QStringList &tags) const;
    QJsonObject buildUrlTest(const QStringList &tags) const;
    QJsonObject buildRoute(const QStringList &tags) const;
    QJsonArray buildRuleSets() const;
    QJsonObject buildExperimental() const;

    QList<VpnNode> m_nodes;
    QString m_selectedTag;
    RoutingMode m_mode = RoutingMode::Smart;
    int m_clashApiPort = 9090;
    QString m_clashApiSecret;
    QString m_tunInterfaceName = QStringLiteral("clgvpn0");
    QString m_tunAddress = QStringLiteral("172.19.0.1/30");
    QString m_tunStack = QStringLiteral("gvisor");
    QString m_ruleSetBaseUrl = QString::fromLatin1(defaultRuleSetBaseUrl);
    QString m_logLevel = QStringLiteral("warn");
    std::optional<int> m_mixedPort;
    std::optional<QString> m_ruleSetDirectory;
};
