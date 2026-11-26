// src/core/app_controller.cpp
#include "app_controller.h"

#include "engine/tunnel_controller.h"
#include "engine/tunnel_state.h"
#include "engine/log_buffer.h"
#include "subscription/subscription_manager.h"
#include "subscription/subscription_service.h"
#include "storage/subscription_store.h"
#include "storage/settings_store.h"
#include "subscription/auto_refresh.h"
#include "util/format.h"
#include "model/vpn_node.h"

#include <QDir>
#include <QTimer>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    // Таймер обновления uptime — раз в секунду эмитим stateChanged.
    m_uptimeTimer.setInterval(1000);
    connect(&m_uptimeTimer, &QTimer::timeout, this, [this]() {
        if (m_tunnel && isConnected(m_tunnel->state().status))
            emit stateChanged();
    });
    m_uptimeTimer.start();
}

AppController::~AppController() = default;

void AppController::init(const QString &dataDir)
{
    m_dataDir = dataDir;
    QDir().mkpath(dataDir);

    m_settingsStore = std::make_unique<SettingsStore>(
        QDir(dataDir).filePath(QStringLiteral("settings.json")));
    m_settings = m_settingsStore->load();

    m_subscriptionStore = std::make_unique<SubscriptionStore>(
        QDir(dataDir).filePath(QStringLiteral("subscriptions.json")));
    m_subscriptionService = std::make_unique<SubscriptionService>();
    m_subscriptions = std::make_unique<SubscriptionsManager>(
        m_subscriptionService.get(), m_subscriptionStore.get());
    m_subscriptions->loadFromStore();

    m_autoRefresh = std::make_unique<SubscriptionAutoRefresh>(m_subscriptions.get());

    m_tunnel = std::make_unique<TunnelController>();
    connect(m_tunnel.get(), &TunnelController::stateChanged,
            this, &AppController::onStateChanged);

    connect(m_subscriptions.get(), &SubscriptionsManager::subscriptionsChanged,
            this, [this]() {
        rebuildNodeLists();
        emit subscriptionsChanged();
    });
    connect(m_subscriptions.get(), &SubscriptionsManager::subscriptionAdded,
            this, [this](const Subscription &sub) {
        emit subscriptionAdded(sub.title, sub.nodes.size());
    });
    connect(m_subscriptions.get(), &SubscriptionsManager::errorOccurred,
            this, [this](const QString &msg) {
        emit errorOccurred(msg);
    });

    m_autoRefresh->start();
    rebuildNodeLists();

    // Если был выбран узел и включён autoConnect — поднимаем туннель.
    if (m_settings.autoConnect && m_settings.selectedNodeTag.has_value()) {
        const VpnNode *node = nullptr;
        for (const auto &n : m_nodesCache) {
            if (n.tag == *m_settings.selectedNodeTag) { node = &n; break; }
        }
        if (node) {
            const VpnNode snapshot = *node;
            const AppSettings settingsSnapshot = m_settings;
            QTimer::singleShot(500, this, [this, snapshot, settingsSnapshot]() {
                m_tunnel->start(snapshot, workingDir(), settingsSnapshot);
            });
        }
    }
}

// --- Tunnel getters ---

int AppController::status() const
{
    return m_tunnel ? static_cast<int>(m_tunnel->state().status) : 0;
}

QString AppController::serverName() const
{
    return m_tunnel ? m_tunnel->state().serverName : QString();
}

QString AppController::statusText() const
{
    if (!m_tunnel) return {};
    switch (m_tunnel->state().status) {
    case TunnelStatus::Disconnected:  return tr("Disconnected");
    case TunnelStatus::Connecting:    return tr("Connecting…");
    case TunnelStatus::Connected:     return tr("Connected");
    case TunnelStatus::Disconnecting: return tr("Disconnecting…");
    }
    return {};
}

qint64 AppController::downloadedBytes() const
{
    return m_tunnel ? m_tunnel->state().downloadedBytes : 0;
}

qint64 AppController::uploadedBytes() const
{
    return m_tunnel ? m_tunnel->state().uploadedBytes : 0;
}

QString AppController::error() const
{
    if (!m_tunnel) return {};
    const auto &e = m_tunnel->state().error;
    return e.has_value() ? *e : QString();
}

QString AppController::hint() const
{
    if (!m_tunnel) return {};
    const auto &h = m_tunnel->state().hint;
    if (!h.has_value()) return {};
    switch (*h) {
    case TunnelHint::TunUnavailable:
        return tr("Proxy mode: TUN needs privileges. Install the clgvpn package "
                  "(it grants CAP_NET_ADMIN) or run as root.");
    case TunnelHint::KillSwitchFailed:
        return tr("Kill switch failed: traffic is not blocked");
    }
    return {};
}

bool AppController::killSwitchActive() const
{
    return m_tunnel && m_tunnel->state().killSwitchActive;
}

QString AppController::uptimeText() const
{
    if (!m_tunnel) return {};
    const auto &ca = m_tunnel->state().connectedAt;
    if (!ca.has_value()) return {};
    const qint64 secs = ca->secsTo(QDateTime::currentDateTimeUtc());
    return formatUptime(secs);
}

QString AppController::modeText() const
{
    if (!m_tunnel) return {};
    const auto &m = m_tunnel->state().mode;
    if (!m.has_value()) return {};
    return *m == TunnelMode::Tun ? tr("TUN (system)") : tr("Proxy (SOCKS/HTTP)");
}

void AppController::toggleConnection()
{
    if (!m_tunnel) return;

    const auto st = m_tunnel->state().status;
    if (st == TunnelStatus::Connected) {
        m_tunnel->disconnect();
        return;
    }
    if (st == TunnelStatus::Disconnected) {
        const auto &tag = m_settings.selectedNodeTag;
        if (!tag.has_value()) {
            emit errorOccurred(tr("Select a server first"));
            return;
        }
        for (const auto &n : m_nodesCache) {
            if (n.tag == *tag) {
                m_tunnel->start(n, workingDir(), m_settings);
                return;
            }
        }
    }
}

void AppController::clearError()
{
    if (m_tunnel) m_tunnel->clearError();
}

void AppController::onStateChanged(const TunnelState &)
{
    emit stateChanged();

    const auto &err = m_tunnel->state().error;
    if (err.has_value() && !err->isEmpty())
        emit errorOccurred(*err);
}

// --- Settings ---

int AppController::routingMode() const
{
    return static_cast<int>(m_settings.routingMode);
}
bool AppController::autoConnect() const { return m_settings.autoConnect; }
bool AppController::killSwitch() const { return m_settings.killSwitch; }
QString AppController::localeCode() const
{
    return m_settings.localeCode.value_or(QString());
}
QString AppController::selectedNodeTag() const
{
    return m_settings.selectedNodeTag.value_or(QString());
}

void AppController::setRoutingMode(int mode)
{
    m_settings.routingMode = static_cast<RoutingMode>(mode);
    saveSettings();
    emit settingsChanged();
}

void AppController::setAutoConnect(bool value)
{
    m_settings.autoConnect = value;
    saveSettings();
    emit settingsChanged();
}

void AppController::setKillSwitch(bool value)
{
    m_settings.killSwitch = value;
    saveSettings();
    emit settingsChanged();
}

void AppController::setLocaleCode(const QString &code)
{
    if (code.isEmpty())
        m_settings.localeCode.reset();
    else
        m_settings.localeCode = code;
    saveSettings();
    emit settingsChanged();
}

void AppController::saveSettings()
{
    if (m_settingsStore)
        m_settingsStore->save(m_settings);
}

// --- Nodes ---

QVariantList AppController::nodes() const
{
    QVariantList out;
    for (const auto &n : m_nodesCache) {
        QVariantMap m;
        m[QStringLiteral("tag")] = n.tag;
        m[QStringLiteral("name")] = n.name;
        m[QStringLiteral("protocol")] = protocolToString(n.protocol);
        m[QStringLiteral("endpoint")] = n.endpointLabel();
        m[QStringLiteral("selected")] = (m_settings.selectedNodeTag == n.tag);
        out.append(m);
    }
    return out;
}

QVariantList AppController::subscriptions() const
{
    QVariantList out;
    if (!m_subscriptions) return out;
    for (const auto &s : m_subscriptions->subscriptions()) {
        QVariantMap m;
        m[QStringLiteral("id")] = s.id;
        m[QStringLiteral("title")] = s.title;
        m[QStringLiteral("nodeCount")] = s.nodes.size();
        m[QStringLiteral("updatedAt")] = s.updatedAt;
        out.append(m);
    }
    return out;
}

QString AppController::selectedNodeName() const
{
    const auto &tag = m_settings.selectedNodeTag;
    if (!tag.has_value()) return {};
    for (const auto &n : m_nodesCache) {
        if (n.tag == *tag) return n.name;
    }
    return {};
}

void AppController::selectNode(const QString &tag)
{
    if (tag.isEmpty()) {
        m_settings.selectedNodeTag.reset();
    } else {
        m_settings.selectedNodeTag = tag;
    }
    saveSettings();
    rebuildNodeLists();
    emit selectedNodeChanged();
    emit settingsChanged();
    emit subscriptionsChanged();   // nodes() тоже зависит от selectedNodeTag
}

void AppController::addSubscription(const QString &url)
{
    if (m_subscriptions) m_subscriptions->add(url);
}

void AppController::refreshSubscription(const QString &id)
{
    if (m_subscriptions) m_subscriptions->refresh(id);
}

void AppController::removeSubscription(const QString &id)
{
    if (m_subscriptions) m_subscriptions->remove(id);
}

void AppController::rebuildNodeLists()
{
    m_nodesCache.clear();
    if (m_subscriptions) {
        for (const auto &sub : m_subscriptions->subscriptions())
            for (const auto &n : sub.nodes)
                m_nodesCache.append(n);
    }
}

// --- Logs ---

QStringList AppController::logs() const
{
    return m_tunnel && m_tunnel->logBuffer()
               ? m_tunnel->logBuffer()->lines()
               : QStringList{};
}

void AppController::clearLogs()
{
    if (m_tunnel && m_tunnel->logBuffer()) {
        m_tunnel->logBuffer()->clear();
        emit logsChanged();
    }
}

// --- Utility ---

QString AppController::formatBytes(qint64 bytes) const
{
    return ::formatBytes(bytes);
}

QString AppController::formatMoment(const QDateTime &dt) const
{
    return ::formatMoment(dt);
}

QString AppController::workingDir() const
{
    return QDir(m_dataDir).filePath(QStringLiteral("runtime"));
}
