// kill_switch.cpp
#include "kill_switch.h"

#include <QProcess>

KillSwitch::KillSwitch(QString helper, QString privilegedLauncher)
    : m_helper(std::move(helper))
    , m_launcher(std::move(privilegedLauncher))
{}

QString KillSwitch::call(const QStringList &args, QString *errorOut)
{
    QProcess p;
    p.start(m_launcher, QStringList{m_helper} + args);
    if (!p.waitForFinished(30000)) {
        if (errorOut)
            *errorOut = QStringLiteral("pkexec не ответил за 30 секунд");
        return {};
    }
    if (p.exitCode() != 0) {
        const QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        if (errorOut) {
            *errorOut = err.isEmpty()
                            ? QStringLiteral("Kill switch завершился с кодом %1")
                                  .arg(p.exitCode())
                            : err;
        }
    }
    return QString::fromUtf8(p.readAllStandardOutput());
}

QString KillSwitch::enable(const QStringList &serverAddresses)
{
    QString err;
    call(QStringList{QStringLiteral("up")} + serverAddresses, &err);
    return err;
}

QString KillSwitch::disable()
{
    QString err;
    call({QStringLiteral("down")}, &err);
    return err;
}

bool KillSwitch::isEnabled()
{
    QString err;
    const QString out = call({QStringLiteral("status")}, &err);
    return err.isEmpty() && out.trimmed() == QStringLiteral("on");
}
