#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/// Запускает sing-box отдельным процессом (Windows/Linux).
class DesktopEngine : public QObject {
    Q_OBJECT
public:
    DesktopEngine(QString workingDir, QString binaryPath,
                  QObject *parent = nullptr);
    ~DesktopEngine() override;

    /// Запускает процесс с готовым конфигом и ждёт Clash API.
    /// Испускает started() или startFailed(QString).
    void start(const QString &configJson, int clashApiPort);

    void stop();

    QStringList recentLogs() const { return m_tail; }

signals:
    void started();
    void startFailed(const QString &error);
    void logLine(const QString &line);
    void exited(int code);

private:
    void onStdout();
    void onStderr();
    void appendLine(const QString &line);

    static constexpr int kTailSize = 200;

    QString m_workingDir;
    QString m_binaryPath;
    QProcess *m_process = nullptr;
    bool m_stopping = false;
    QStringList m_tail;
};
