#include "desktop_engine.h"
#include "clash_api.h"

#include <QDir>
#include <QFile>
#include <QDebug>

DesktopEngine::DesktopEngine(QString workingDir, QString binaryPath,
                             QObject *parent)
    : QObject(parent)
    , m_workingDir(std::move(workingDir))
    , m_binaryPath(std::move(binaryPath))
{}

DesktopEngine::~DesktopEngine() { stop(); }

void DesktopEngine::start(const QString &configJson, int clashApiPort)
{
    if (m_process) {
        emit startFailed(QStringLiteral("Ядро уже запущено"));
        return;
    }

    QDir().mkpath(m_workingDir);
    const QString configPath =
        QDir(m_workingDir).filePath(QStringLiteral("config.json"));
    {
        QFile f(configPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit startFailed(QStringLiteral("Не удалось записать конфиг: %1")
                                 .arg(configPath));
            return;
        }
        f.write(configJson.toUtf8());
    }

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDir);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &DesktopEngine::onStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &DesktopEngine::onStderr);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
        if (!m_stopping)
            appendLine(QStringLiteral("Ядро завершилось с кодом %1").arg(code));
        emit exited(code);
    });

    m_process->start(m_binaryPath,
                     {QStringLiteral("run"), QStringLiteral("-c"), configPath});
    if (!m_process->waitForStarted(5000)) {
        m_process->deleteLater();
        m_process = nullptr;
        emit startFailed(QStringLiteral("Не удалось запустить ядро: %1")
                             .arg(m_binaryPath));
        return;
    }

    auto *api = new ClashApiClient(clashApiPort, {}, {}, this);
    connect(api, &ClashApiClient::ready, this, [this, api]() {
        api->deleteLater();
        emit started();
    });
    connect(api, &ClashApiClient::readyFailed, this,
            [this, api](const QString &err) {
        api->deleteLater();
        const QString tailText = m_tail.join(QLatin1Char('\n'));
        stop();
        emit startFailed(QStringLiteral("Ядро не поднялось: %1\n%2")
                             .arg(err, tailText));
    });
    api->waitReady();
}

void DesktopEngine::stop()
{
    if (!m_process) return;
    m_stopping = true;
    m_process->terminate();
    if (!m_process->waitForFinished(5000)) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
    if (m_process) { m_process->deleteLater(); m_process = nullptr; }
    m_stopping = false;
}

void DesktopEngine::onStdout()
{
    if (!m_process) return;
    const QString text = QString::fromUtf8(m_process->readAllStandardOutput());
    for (const auto &line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        appendLine(line);
}

void DesktopEngine::onStderr()
{
    if (!m_process) return;
    const QString text = QString::fromUtf8(m_process->readAllStandardError());
    for (const auto &line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        appendLine(line);
}

void DesktopEngine::appendLine(const QString &line)
{
    if (line.trimmed().isEmpty()) return;
    m_tail.append(line);
    while (m_tail.size() > kTailSize) m_tail.removeFirst();
    emit logLine(line);
}
