#include "core_install.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <stdexcept>

namespace {

QString osName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

QString archName()
{
#if defined(Q_PROCESSOR_X86_64)
    return QStringLiteral("amd64");
#elif defined(Q_PROCESSOR_ARM_64)
    return QStringLiteral("arm64");
#elif defined(Q_PROCESSOR_ARM)
    return QStringLiteral("arm");
#else
    return QStringLiteral("amd64");
#endif
}

QString binaryName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("sing-box.exe");
#else
    return QStringLiteral("sing-box");
#endif
}

} // namespace

CoreInstall CoreInstall::resolve()
{
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString cwd = QDir::currentPath();
    const QString distName =
        QStringLiteral("%1-%2").arg(osName(), archName());

    QStringList candidates;
    const QString env = QProcessEnvironment::systemEnvironment()
                            .value(QString::fromLatin1(envBinary));
    if (!env.isEmpty()) candidates << env;

    candidates
        << QDir(exeDir).filePath(binaryName())
        << QDir(exeDir).filePath(
               QStringLiteral("../core/dist/%1/%2").arg(distName, binaryName()))
        << QDir(cwd).filePath(
               QStringLiteral("core/dist/%1/%2").arg(distName, binaryName()))
        << QDir(cwd).filePath(
               QStringLiteral("../core/dist/%1/%2").arg(distName, binaryName()));

    QString binary;
    for (const auto &c : candidates) {
        const QString normalized =
            QDir::cleanPath(QFileInfo(c).absoluteFilePath());
        if (QFileInfo::exists(normalized)) {
            binary = normalized;
            break;
        }
    }

    if (binary.isEmpty()) {
        throw std::runtime_error(
            QStringLiteral("Ядро sing-box не найдено. Проверены пути:\n%1\n"
                           "Соберите его: core/build_desktop.sh %2 %3")
                .arg(candidates.join(QLatin1Char('\n')), osName(), archName())
                .toStdString());
    }

    std::optional<QString> ruleSets;
    const QStringList dirCandidates = {
        QDir(exeDir).filePath(QStringLiteral("rulesets")),
        QDir(cwd).filePath(QStringLiteral("rulesets/dist")),
        QDir(cwd).filePath(QStringLiteral("../rulesets/dist")),
    };
    for (const auto &dir : dirCandidates) {
        const QString domain = QDir(dir).filePath(
            QStringLiteral("geosite-category-ru.srs"));
        const QString cidr = QDir(dir).filePath(QStringLiteral("geoip-ru.srs"));
        if (QFileInfo::exists(domain) && QFileInfo::exists(cidr)) {
            ruleSets = QDir::cleanPath(dir);
            break;
        }
    }

    return CoreInstall{binary, ruleSets};
}
