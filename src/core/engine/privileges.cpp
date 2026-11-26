// privileges.cpp
#include "privileges.h"

#include <QProcess>
#include <QStandardPaths>

bool Privileges::canCreateTun(const QString &binaryPath, const QString &getcap)
{
#ifdef Q_OS_LINUX
    QProcess p;
    p.start(getcap, {binaryPath});
    if (!p.waitForFinished(3000)) return false;
    if (p.exitCode() != 0) return false;

    const QString out = QString::fromUtf8(p.readAllStandardOutput()).toLower();
    return out.contains(QStringLiteral("cap_net_admin"));
#else
    Q_UNUSED(binaryPath);
    Q_UNUSED(getcap);
    return false;
#endif
}
