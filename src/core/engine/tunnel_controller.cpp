#include "tunnel_controller.h"

#include "desktop_engine.h"
#include "clash_api.h"
#include "core_install.h"
#include "privileges.h"
#include "kill_switch.h"
#include "log_buffer.h"
#include "../singbox/config_builder.h"

#include <QJsonDocument>
#include <QTcpServer>
#include <QDebug>

TunnelController::TunnelController(QObject *parent)
    : QObject(parent)
    , m_logs(new LogBuffer(this))
    , m_killSwitch(std::make_unique<KillSwitch>())
{
    m_statsTimer.setInterval(2000);
    connect(&m_statsTimer, &QTimer::timeout, this, [this]() {
        if (m_api && isConnected(m_state.status))
            m_api->fetchTotals();
    });
}

TunnelController::~TunnelController()
{
    if (m_killSwitchActive)
        m_killSwitch->disable();
    teardown();
}

void TunnelController::setState(TunnelState next)
{
    m_state = std::move(next);
    emit stateChanged(m_state);
}

void TunnelController::start(const VpnNode &node,
                             const QString &workingDir,
                             const AppSettings &settings)
{
    qDebug() << "=== TunnelController::start ===";
    qDebug() << "  node:" << node.name << "tag:" << node.tag;
    qDebug() << "  workingDir:" << workingDir;

    if (isBusy(m_state.status) || isConnected(m_state.status)) {
        qDebug() << "  already busy/connected, skipping";
        return;
    }

    m_currentNode = node;

    TunnelState s;
    s.status = TunnelStatus::Connecting;
    s.serverName = node.name;
    setState(s);

    CoreInstall install;
    try {
        install = CoreInstall::resolve();
    } catch (const std::exception &e) {
        qDebug() << "  CoreInstall::resolve failed:" << e.what();
        s.status = TunnelStatus::Disconnected;
        s.error = QString::fromStdString(e.what());
        setState(s);
        return;
    }
    qDebug() << "  binary:" << install.binary;
    qDebug() << "  ruleSetsDir:" << install.ruleSetsDir.value_or(QStringLiteral("<none>"));

    const bool proxyMode = !Privileges::canCreateTun(install.binary);
    const TunnelMode mode = proxyMode ? TunnelMode::Proxy : TunnelMode::Tun;
    const int clashPort = freePort();
    qDebug() << "  proxyMode:" << proxyMode << "mode:" << static_cast<int>(mode);
    qDebug() << "  clashPort:" << clashPort;

    SingBoxConfigBuilder builder;
    builder.setNodes({node})
        .setSelectedTag(node.tag)
        .setMode(settings.routingMode)
        .setClashApiPort(clashPort)
        .setLogLevel(QStringLiteral("debug"));   // временно, для диагностики

    if (proxyMode)
        builder.setMixedPort(kDesktopMixedPort);
    if (install.ruleSetsDir.has_value())
        builder.setRuleSetDirectory(*install.ruleSetsDir);

    const QJsonObject cfg = builder.build();
    const QByteArray cfgJson =
        QJsonDocument(cfg).toJson(QJsonDocument::Compact);

    m_engine = new DesktopEngine(workingDir, install.binary, this);
    connect(m_engine, &DesktopEngine::logLine,
            m_logs, &LogBuffer::add);

    connect(m_engine, &DesktopEngine::started, this,
            [this, clashPort, mode, settings, node, proxyMode]() {
        qDebug() << "=== DesktopEngine::started ===";

        m_api = new ClashApiClient(clashPort, {}, {}, this);
        connect(m_api, &ClashApiClient::totalsReceived, this,
                [this](qint64 down, qint64 up) {
            m_state.downloadedBytes = down;
            m_state.uploadedBytes = up;
            emit stateChanged(m_state);
        });

        TunnelState st = m_state;
        st.status = TunnelStatus::Connected;
        st.clashApiPort = clashPort;
        st.mode = mode;
        st.connectedAt = QDateTime::currentDateTimeUtc();
        if (proxyMode) st.hint = TunnelHint::TunUnavailable;
        setState(st);

        if (settings.killSwitch && mode == TunnelMode::Tun)
            enableKillSwitch(node.server);

        startStats();
    });

    connect(m_engine, &DesktopEngine::startFailed, this,
            [this](const QString &error) {
        qDebug() << "=== DesktopEngine::startFailed ===" << error;
        teardown();
        TunnelState st;
        st.status = TunnelStatus::Disconnected;
        st.error = error;
        st.killSwitchActive = false;
        setState(st);
    });

    m_engine->start(QString::fromUtf8(cfgJson), clashPort);
}

void TunnelController::disconnect()
{
    if (!isConnected(m_state.status)) return;

    TunnelState s = m_state;
    s.status = TunnelStatus::Disconnecting;
    setState(s);

    teardown();

    TunnelState finalState;
    finalState.status = TunnelStatus::Disconnected;
    setState(finalState);
}

void TunnelController::clearError()
{
    if (!m_state.error.has_value()) return;
    m_state.error.reset();
    emit stateChanged(m_state);
}

void TunnelController::enableKillSwitch(const QString &serverAddress)
{
    const QString err = m_killSwitch->enable({serverAddress});
    if (err.isEmpty()) {
        m_killSwitchActive = true;
        m_state.killSwitchActive = true;
        emit stateChanged(m_state);
    } else {
        m_state.hint = TunnelHint::KillSwitchFailed;
        m_state.error = err;
        emit stateChanged(m_state);
    }
}

void TunnelController::teardown()
{
    if (m_killSwitchActive) {
        m_killSwitch->disable();
        m_killSwitchActive = false;
    }
    m_statsTimer.stop();
    if (m_api) {
        m_api->deleteLater();
        m_api = nullptr;
    }
    if (m_engine) {
        m_engine->stop();
        m_engine->deleteLater();
        m_engine = nullptr;
    }
}

void TunnelController::startStats()
{
    m_statsTimer.start();
}

int TunnelController::freePort()
{
    QTcpServer s;
    s.listen(QHostAddress::LocalHost, 0);
    const int port = s.serverPort();
    s.close();
    return port;
}
