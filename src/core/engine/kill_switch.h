// kill_switch.h
#pragma once

#include <QString>
#include <QStringList>

/// Управляет системным kill switch через helper из пакета.
///
/// Helper — `clgvpn-killswitch` с nftables-таблицей; запускается через pkexec,
/// потому что правила netfilter требуют root. Пакет ставит polkit-правило,
/// поэтому запрос пароля не появляется.
class KillSwitch {
public:
    explicit KillSwitch(QString helper = defaultHelper(),
                        QString privilegedLauncher = QStringLiteral("pkexec"));

    /// Включает блокировку, разрешая только адреса VPN-сервера.
    /// Возвращает пустую строку при успехе или текст ошибки.
    QString enable(const QStringList &serverAddresses);
    QString disable();
    bool isEnabled();

    static QString defaultHelper()
    { return QStringLiteral("/usr/lib/clgvpn/clgvpn-killswitch"); }

private:
    QString call(const QStringList &args, QString *errorOut);

    QString m_helper;
    QString m_launcher;
};
