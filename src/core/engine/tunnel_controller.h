#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <memory>

#include "tunnel_state.h"
#include "../model/vpn_node.h"
#include "../settings/app_settings.h"

class DesktopEngine;
class ClashApiClient;
class KillSwitch;
class LogBuffer;

/// Порт локального SOCKS/HTTP для десктопного режима без прав администратора.
inline constexpr int kDesktopMixedPort = 11085;

/// Оркестратор туннеля: запуск ядра, статистика, kill switch, ошибки.
///
/// Метод запуска называется start(), а не connect(): QObject::connect
/// зарезервирован и перекрывать его в подклассе нельзя.
class TunnelController : public QObject {
    Q_OBJECT

public:
    explicit TunnelController(QObject *parent = nullptr);
    ~TunnelController() override;

    TunnelState state() const { return m_state; }
    LogBuffer *logBuffer() const { return m_logs; }

    /// Запускает туннель. workingDir — каталог для config.json ядра.
    void start(const VpnNode &node,
               const QString &workingDir,
               const AppSettings &settings);

    void disconnect();
    void clearError();

signals:
    void stateChanged(const TunnelState &state);

private:
    void setState(TunnelState next);
    void teardown();
    void startStats();
    void enableKillSwitch(const QString &serverAddress);
    static int freePort();

    TunnelState m_state;
    DesktopEngine *m_engine = nullptr;
    ClashApiClient *m_api = nullptr;
    QTimer m_statsTimer;
    LogBuffer *m_logs = nullptr;
    std::unique_ptr<KillSwitch> m_killSwitch;
    bool m_killSwitchActive = false;
    VpnNode m_currentNode;
};
