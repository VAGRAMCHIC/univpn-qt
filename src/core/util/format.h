#pragma once

#include <QString>
#include <QDateTime>

/// 1536 -> "1.5 KB"
QString formatBytes(qint64 bytes, int decimals = 1);

QString formatDate(const QDateTime &date);   // 05.12.2026
QString formatTime(const QDateTime &date);   // 14:03
QString formatMoment(const QDateTime &date); // сегодня -> время, иначе дата

/// Время работы туннеля: "05:12", "2:03:44".
QString formatUptime(qint64 seconds);
