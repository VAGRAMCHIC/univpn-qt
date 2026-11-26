#pragma once

#include <QString>
#include <QDateTime>
#include <optional>

enum class TunnelMode { Tun, Proxy };
enum class TunnelHint { TunUnavailable, KillSwitchFailed };

enum class TunnelStatus {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
};

inline bool isBusy(TunnelStatus s)
{ return s == TunnelStatus::Connecting || s == TunnelStatus::Disconnecting; }
inline bool isConnected(TunnelStatus s) { return s == TunnelStatus::Connected; }

struct TunnelState {
    TunnelStatus status = TunnelStatus::Disconnected;
    QString serverName;
    qint64 downloadedBytes = 0;
    qint64 uploadedBytes = 0;
    std::optional<QString> error;
    std::optional<TunnelHint> hint;
    std::optional<int> clashApiPort;
    std::optional<TunnelMode> mode;
    std::optional<QDateTime> connectedAt;
    bool killSwitchActive = false;
};
