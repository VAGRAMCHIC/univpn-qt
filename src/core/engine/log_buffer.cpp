// log_buffer.cpp
#include "log_buffer.h"

LogBuffer::LogBuffer(QObject *parent) : QObject(parent) {}

void LogBuffer::add(const QString &line) { addAll({line}); }

void LogBuffer::addAll(const QStringList &lines)
{
    bool changedFlag = false;
    for (const auto &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        m_lines.append(line);
        changedFlag = true;
    }
    while (m_lines.size() > kLimit)
        m_lines.removeFirst();
    if (changedFlag) emit changed();
}

void LogBuffer::clear()
{
    if (m_lines.isEmpty()) return;
    m_lines.clear();
    emit changed();
}
