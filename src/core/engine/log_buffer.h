// log_buffer.h
#pragma once

#include <QObject>
#include <QStringList>

/// Кольцевой буфер логов ядра для экрана диагностики.
class LogBuffer : public QObject {
    Q_OBJECT

public:
    explicit LogBuffer(QObject *parent = nullptr);

    QStringList lines() const { return m_lines; }

    void add(const QString &line);
    void addAll(const QStringList &lines);
    void clear();

    static constexpr int kLimit = 500;

signals:
    void changed();

private:
    QStringList m_lines;
};
