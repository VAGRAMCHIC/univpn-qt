#include "config_builder.h"
#include <QDir>

QString routingModeToString(RoutingMode mode)
{
    switch (mode) {
    case RoutingMode::Smart:     return QStringLiteral("smart");
    case RoutingMode::Global:    return QStringLiteral("global");
    case RoutingMode::Whitelist: return QStringLiteral("whitelist");
    }
    return {};
}

RoutingMode routingModeFromString(const QString &value)
{
    if (value == QLatin1String("global"))    return RoutingMode::Global;
    if (value == QLatin1String("whitelist")) return RoutingMode::Whitelist;
    return RoutingMode::Smart;
}

// --- Сеттеры (fluent API) ---

SingBoxConfigBuilder &SingBoxConfigBuilder::setNodes(QList<VpnNode> nodes)
{ m_nodes = std::move(nodes); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setSelectedTag(QString tag)
{ m_selectedTag = std::move(tag); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setMode(RoutingMode mode)
{ m_mode = mode; return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setClashApiPort(int port)
{ m_clashApiPort = port; return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setClashApiSecret(QString secret)
{ m_clashApiSecret = std::move(secret); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setTunInterfaceName(QString name)
{ m_tunInterfaceName = std::move(name); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setTunAddress(QString address)
{ m_tunAddress = std::move(address); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setTunStack(QString stack)
{ m_tunStack = std::move(stack); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setRuleSetBaseUrl(QString url)
{ m_ruleSetBaseUrl = std::move(url); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setLogLevel(QString level)
{ m_logLevel = std::move(level); return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setMixedPort(std::optional<int> port)
{ m_mixedPort = port; return *this; }

SingBoxConfigBuilder &SingBoxConfigBuilder::setRuleSetDirectory(std::optional<QString> dir)
{ m_ruleSetDirectory = std::move(dir); return *this; }

// --- build ---

QJsonObject SingBoxConfigBuilder::build() const
{
    QStringList tags;
    tags.reserve(m_nodes.size());
    for (const auto &node : m_nodes)
        tags.append(node.tag);

    QJsonObject log;
    log[QStringLiteral("level")]     = m_logLevel;
    log[QStringLiteral("timestamp")] = true;

    QJsonArray outbounds;
    for (const auto &node : m_nodes)
        outbounds.append(node.toOutbound());

    {
        QJsonObject direct;
        direct[QStringLiteral("type")] = QStringLiteral("direct");
        direct[QStringLiteral("tag")]  = QStringLiteral("direct");
        outbounds.append(direct);
    }
    {
        QJsonObject block;
        block[QStringLiteral("type")] = QStringLiteral("block");
        block[QStringLiteral("tag")]  = QStringLiteral("block");
        outbounds.append(block);
    }
    outbounds.append(buildSelector(tags));
    outbounds.append(buildUrlTest(tags));

    QJsonArray inbounds;
    if (m_mixedPort.has_value())
        inbounds.append(buildMixedInbound(*m_mixedPort));
    else
        inbounds.append(buildTunInbound());

    QJsonObject result;
    result[QStringLiteral("log")]          = log;
    result[QStringLiteral("dns")]          = buildDns();
    result[QStringLiteral("inbounds")]     = inbounds;
    result[QStringLiteral("outbounds")]    = outbounds;
    result[QStringLiteral("route")]        = buildRoute(tags);
    result[QStringLiteral("experimental")] = buildExperimental();
    return result;
}

QJsonObject SingBoxConfigBuilder::buildDns() const
{
    // Формат DNS-серверов sing-box 1.12+: legacy `address` удалён в 1.14.
    QJsonObject directDns;
    directDns[QStringLiteral("type")]   = QStringLiteral("https");
    directDns[QStringLiteral("tag")]    = QStringLiteral("dns-direct");
    directDns[QStringLiteral("server")] = QStringLiteral("77.88.8.8");
    directDns[QStringLiteral("path")]   = QStringLiteral("/dns-query");

    QJsonObject remoteDns;
    remoteDns[QStringLiteral("type")]   = QStringLiteral("https");
    remoteDns[QStringLiteral("tag")]    = QStringLiteral("dns-remote");
    remoteDns[QStringLiteral("server")] = QStringLiteral("1.1.1.1");
    remoteDns[QStringLiteral("path")]   = QStringLiteral("/dns-query");
    remoteDns[QStringLiteral("detour")] = QStringLiteral("proxy");

    QJsonArray rules;
    if (m_mode != RoutingMode::Global) {
        QJsonObject rule;
        rule[QStringLiteral("rule_set")] = QJsonArray{QStringLiteral("geosite-ru")};
        rule[QStringLiteral("server")]   = QStringLiteral("dns-direct");
        rules.append(rule);
    }

    QJsonObject result;
    result[QStringLiteral("servers")] = QJsonArray{directDns, remoteDns};
    result[QStringLiteral("rules")]   = rules;
    result[QStringLiteral("final")]   = QStringLiteral("dns-remote");
    return result;
}

QJsonObject SingBoxConfigBuilder::buildTunInbound() const
{
    QJsonObject o;
    o[QStringLiteral("type")]           = QStringLiteral("tun");
    o[QStringLiteral("tag")]            = QStringLiteral("tun-in");
    o[QStringLiteral("interface_name")] = m_tunInterfaceName;
    o[QStringLiteral("address")]        = QJsonArray{m_tunAddress};
    o[QStringLiteral("mtu")]            = 9000;
    o[QStringLiteral("auto_route")]     = true;
    o[QStringLiteral("strict_route")]   = true;
    o[QStringLiteral("stack")]          = m_tunStack;
    return o;
}

QJsonObject SingBoxConfigBuilder::buildMixedInbound(int port) const
{
    QJsonObject o;
    o[QStringLiteral("type")]        = QStringLiteral("mixed");
    o[QStringLiteral("tag")]         = QStringLiteral("mixed-in");
    o[QStringLiteral("listen")]      = QStringLiteral("127.0.0.1");
    o[QStringLiteral("listen_port")] = port;
    return o;
}

QJsonObject SingBoxConfigBuilder::buildSelector(const QStringList &tags) const
{
    QJsonArray outbounds;
    outbounds.append(QStringLiteral("urltest"));
    for (const auto &tag : tags)
        outbounds.append(tag);

    QJsonObject o;
    o[QStringLiteral("type")]      = QStringLiteral("selector");
    o[QStringLiteral("tag")]       = QStringLiteral("proxy");
    o[QStringLiteral("outbounds")] = outbounds;
    o[QStringLiteral("default")]   = QStringLiteral("urltest");
    return o;
}

QJsonObject SingBoxConfigBuilder::buildUrlTest(const QStringList &tags) const
{
    QJsonArray outbounds;
    for (const auto &tag : tags)
        outbounds.append(tag);

    QJsonObject o;
    o[QStringLiteral("type")]      = QStringLiteral("urltest");
    o[QStringLiteral("tag")]       = QStringLiteral("urltest");
    o[QStringLiteral("outbounds")] = outbounds;
    o[QStringLiteral("url")]       = QStringLiteral("https://www.gstatic.com/generate_204");
    o[QStringLiteral("interval")]  = QStringLiteral("3m");
    o[QStringLiteral("tolerance")] = 50;
    return o;
}

QJsonObject SingBoxConfigBuilder::buildRoute(const QStringList &tags) const
{
    QJsonArray rules;
    {
        QJsonObject sniff;
        sniff[QStringLiteral("action")] = QStringLiteral("sniff");
        rules.append(sniff);
    }
    {
        QJsonObject dns;
        dns[QStringLiteral("protocol")] = QStringLiteral("dns");
        dns[QStringLiteral("action")]   = QStringLiteral("hijack-dns");
        rules.append(dns);
    }

    if (m_mode == RoutingMode::Smart) {
        {
            QJsonObject priv;
            priv[QStringLiteral("ip_is_private")] = true;
            priv[QStringLiteral("outbound")]      = QStringLiteral("direct");
            rules.append(priv);
        }
        {
            QJsonObject geoip;
            geoip[QStringLiteral("rule_set")] = QJsonArray{QStringLiteral("geoip-ru")};
            geoip[QStringLiteral("outbound")] = QStringLiteral("direct");
            rules.append(geoip);
        }
        {
            QJsonObject geosite;
            geosite[QStringLiteral("rule_set")] = QJsonArray{QStringLiteral("geosite-ru")};
            geosite[QStringLiteral("outbound")] = QStringLiteral("direct");
            rules.append(geosite);
        }
    } else if (m_mode == RoutingMode::Whitelist) {
        QJsonObject priv;
        priv[QStringLiteral("ip_is_private")] = true;
        priv[QStringLiteral("outbound")]      = QStringLiteral("direct");
        rules.append(priv);
    }

    QJsonObject o;
    o[QStringLiteral("rules")] = rules;
    if (m_mode == RoutingMode::Global)
        o[QStringLiteral("rule_set")] = QJsonArray();
    else
        o[QStringLiteral("rule_set")] = buildRuleSets();
    o[QStringLiteral("final")] =
        (m_mode == RoutingMode::Whitelist)
            ? QStringLiteral("direct")
            : QStringLiteral("proxy");
    o[QStringLiteral("default_domain_resolver")] = QStringLiteral("dns-direct");
    o[QStringLiteral("auto_detect_interface")]   = true;
    return o;
}


QJsonArray SingBoxConfigBuilder::buildRuleSets() const
{
    QJsonArray arr;

    // Вариант 1: локальные файлы (если папка найдена CoreInstall)
    if (m_ruleSetDirectory.has_value()) {
        const QString dir = *m_ruleSetDirectory;

        auto makeLocal = [&](const QString &tag, const QString &file) {
            QJsonObject o;
            o[QStringLiteral("type")]   = QStringLiteral("local");
            o[QStringLiteral("tag")]    = tag;
            o[QStringLiteral("format")] = QStringLiteral("binary");
            o[QStringLiteral("path")]   = QDir(dir).filePath(file);
            return o;
        };

        arr.append(makeLocal(QStringLiteral("geosite-ru"),
                             QStringLiteral("geosite-category-ru.srs")));
        arr.append(makeLocal(QStringLiteral("geoip-ru"),
                             QStringLiteral("geoip-ru.srs")));
        return arr;
    }

    // Вариант 2: удалённые — официальные от SagerNet
    auto makeRemote = [&](const QString &tag, const QString &url) {
        QJsonObject o;
        o[QStringLiteral("type")]            = QStringLiteral("remote");
        o[QStringLiteral("tag")]             = tag;
        o[QStringLiteral("format")]          = QStringLiteral("binary");
        o[QStringLiteral("url")]             = url;
        o[QStringLiteral("update_interval")] = QStringLiteral("1d");
        return o;
    };

    arr.append(makeRemote(QStringLiteral("geosite-ru"),
        QStringLiteral("https://raw.githubusercontent.com/SagerNet/sing-geosite/rule-set/geosite-category-ru.srs")));
    arr.append(makeRemote(QStringLiteral("geoip-ru"),
        QStringLiteral("https://raw.githubusercontent.com/SagerNet/sing-geoip/rule-set/geoip-ru.srs")));

    return arr;
}


QJsonObject SingBoxConfigBuilder::buildExperimental() const
{
    QJsonObject api;
    api[QStringLiteral("external_controller")] =
        QStringLiteral("127.0.0.1:%1").arg(m_clashApiPort);
    api[QStringLiteral("secret")] = m_clashApiSecret;

    QJsonObject o;
    o[QStringLiteral("clash_api")] = api;
    // cache_file полностью убран
    return o;
}
