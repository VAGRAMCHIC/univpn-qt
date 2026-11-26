#include "format.h"

#include <array>

QString formatBytes(qint64 bytes, int decimals)
{
    if (bytes <= 0)
        return QStringLiteral("0 B");

    static constexpr std::array<const char *, 5> units = {
        "B", "KB", "MB", "GB", "TB"
    };

    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < static_cast<int>(units.size()) - 1) {
        value /= 1024.0;
        ++unit;
    }

    const int digits = (unit == 0) ? 0 : decimals;
    return QStringLiteral("%1 %2")
        .arg(QString::number(value, 'f', digits))
        .arg(QString::fromLatin1(units[unit]));
}

static QString two(int v)
{
    return QString::number(v).rightJustified(2, QLatin1Char('0'));
}

QString formatDate(const QDateTime &date)
{
    return QStringLiteral("%1.%2.%3")
        .arg(two(date.date().day()))
        .arg(two(date.date().month()))
        .arg(date.date().year());
}

QString formatTime(const QDateTime &date)
{
    return QStringLiteral("%1:%2")
        .arg(two(date.time().hour()))
        .arg(two(date.time().minute()));
}

QString formatMoment(const QDateTime &date)
{
    const QDate today = QDate::currentDate();
    const bool sameDay = (today == date.date());
    return sameDay ? formatTime(date) : formatDate(date);
}

QString formatUptime(qint64 seconds)
{
    const qint64 hours = seconds / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    const qint64 secs = seconds % 60;

    if (hours > 0)
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(two(static_cast<int>(minutes)))
            .arg(two(static_cast<int>(secs)));

    return QStringLiteral("%1:%2")
        .arg(two(static_cast<int>(minutes)))
        .arg(two(static_cast<int>(secs)));
}
