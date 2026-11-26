// src/core/app_controller.h
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantList>
#include <QTimer>
#include <memory>

#include "engine/tunnel_controller.h"
#include "engine/tunnel_state.h"
#include "settings/app_settings.h"
#include "model/vpn_node.h"

class SubscriptionsManager;
class SubscriptionService;
class SubscriptionStore;
class SettingsStore;
class SubscriptionAutoRefresh;

class AppController : public QObject {
    Q_OBJECT

    // --- Tunnel state (для QML) ---
    Q_PROPERTY(int status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString serverName READ serverName NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(qint64 downloadedBytes READ downloadedBytes NOTIFY stateChanged)
    Q_PROPERTY(qint64 uploadedBytes READ uploadedBytes NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString hint READ hint NOTIFY stateChanged)
    Q_PROPERTY(bool killSwitchActive READ killSwitchActive NOTIFY stateChanged)
    Q_PROPERTY(QString uptimeText READ uptimeText NOTIFY stateChanged)
    Q_PROPERTY(QString modeText READ modeText NOTIFY stateChanged)

    // --- Settings ---
    Q_PROPERTY(int routingMode READ routingMode NOTIFY settingsChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect NOTIFY settingsChanged)
    Q_PROPERTY(bool killSwitch READ killSwitch NOTIFY settingsChanged)
    Q_PROPERTY(QString localeCode READ localeCode NOTIFY settingsChanged)
    Q_PROPERTY(QString selectedNodeTag READ selectedNodeTag NOTIFY settingsChanged)

    // --- Nodes / subscriptions ---
    Q_PROPERTY(QVariantList nodes READ nodes NOTIFY subscriptionsChanged)
    Q_PROPERTY(QVariantList subscriptions READ subscriptions NOTIFY subscriptionsChanged)
    Q_PROPERTY(QString selectedNodeName READ selectedNodeName NOTIFY selectedNodeChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    /// Инициализация: загружает настройки, подписки, запускает автообновление.
    void init(const QString &dataDir);

    // --- Tunnel getters ---
    int status() const;
    QString serverName() const;
    QString statusText() const;
    qint64 downloadedBytes() const;
    qint64 uploadedBytes() const;
    QString error() const;
    QString hint() const;
    bool killSwitchActive() const;
    QString uptimeText() const;
    QString modeText() const;

    Q_INVOKABLE void toggleConnection();
    Q_INVOKABLE void clearError();

    // --- Settings ---
    int routingMode() const;
    bool autoConnect() const;
    bool killSwitch() const;
    QString localeCode() const;
    QString selectedNodeTag() const;

    Q_INVOKABLE void setRoutingMode(int mode);
    Q_INVOKABLE void setAutoConnect(bool value);
    Q_INVOKABLE void setKillSwitch(bool value);
    Q_INVOKABLE void setLocaleCode(const QString &code);

    // --- Nodes ---
    QVariantList nodes() const;
    QVariantList subscriptions() const;
    QString selectedNodeName() const;

    Q_INVOKABLE void selectNode(const QString &tag);
    Q_INVOKABLE void addSubscription(const QString &url);
    Q_INVOKABLE void refreshSubscription(const QString &id);
    Q_INVOKABLE void removeSubscription(const QString &id);

    // --- Logs ---
    Q_INVOKABLE QStringList logs() const;
    Q_INVOKABLE void clearLogs();

    // --- Utility ---
    Q_INVOKABLE QString formatBytes(qint64 bytes) const;
    Q_INVOKABLE QString formatMoment(const QDateTime &dt) const;

signals:
    void stateChanged();
    void settingsChanged();
    void subscriptionsChanged();
    void selectedNodeChanged();
    void logsChanged();
    void errorOccurred(const QString &message);
    void subscriptionAdded(const QString &title, int nodeCount);

private:
    void onStateChanged(const TunnelState &state);
    void rebuildNodeLists();
    void saveSettings();
    QString workingDir() const;

    std::unique_ptr<SubscriptionService> m_subscriptionService;
    std::unique_ptr<SubscriptionStore> m_subscriptionStore;
    std::unique_ptr<SettingsStore> m_settingsStore;
    std::unique_ptr<SubscriptionsManager> m_subscriptions;
    std::unique_ptr<SubscriptionAutoRefresh> m_autoRefresh;
    std::unique_ptr<TunnelController> m_tunnel;

    AppSettings m_settings;
    QList<VpnNode> m_nodesCache;
    QString m_dataDir;
    QTimer m_uptimeTimer;
};
