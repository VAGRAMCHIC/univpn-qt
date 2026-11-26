#include "app_paths.h"

#include <QStandardPaths>
#include <QDir>

QString appSupportDir()
{
    const QString base = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    return QDir(base).filePath(QStringLiteral("core"));
}

QString ensureAppSupportDir()
{
    const QString path = appSupportDir();
    if (QDir().mkpath(path))
        return path;
    return {};
}
