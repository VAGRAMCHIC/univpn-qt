#include "subscription_parser.h"
#include "../util/base64.h"
#include "link_parser.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <stdexcept>

#include <QTimeZone>

namespace {

[[noreturn]] void fail(const QString &message)
{
    throw std::invalid_argument(message.toStdString());
}

QString decodeTitle(const QString &raw)
{
    if (raw.isEmpty()) return {};
    if (!raw.startsWith(QLatin1String("base64:")))
        return raw;
    const auto decoded = tryDecodeBase64(raw.mid(7).trimmed());
    return decoded.value_or(QString());
}

std::optional<SubscriptionTraffic> parseUserInfo(const QString &raw)
{
    if (raw.trimmed().isEmpty()) return std::nullopt;

    QMap<QString, qint64> values;
    QRegularExpression re(QStringLiteral(R"((\w+)\s*=\s*(-?\d+))"));
    auto it = re.globalMatch(raw);
    while (it.hasNext()) {
        const auto m = it.next();
        values[m.captured(1)] = m.captured(2).toLongLong();
    }
    if (values.isEmpty()) return std::nullopt;

    SubscriptionTraffic t;
    t.upload   = values.value(QStringLiteral("upload"), 0);
    t.download = values.value(QStringLiteral("download"), 0);
    t.total    = values.value(QStringLiteral("total"), 0);

    const qint64 expire = values.value(QStringLiteral("expire"), 0);
    if (expire > 0)
        t.expire = QDateTime::fromSecsSinceEpoch(expire, QTimeZone::UTC);

    return t;
}

std::optional<int> parseInterval(const QString &raw)
{
    bool ok = false;
    const int hours = raw.trimmed().toInt(&ok);
    if (!ok || hours <= 0) return std::nullopt;
    return hours;
}

} // namespace

SubscriptionMeta SubscriptionMeta::parse(const QMap<QString, QString> &headers)
{
    // Приводим ключи к нижнему регистру, как в Dart.
    QMap<QString, QString> lower;
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        lower.insert(it.key().toLower(), it.value());

    SubscriptionMeta meta;

    const QString title = decodeTitle(lower.value(QStringLiteral("profile-title")));
    if (!title.isEmpty()) meta.title = title;

    meta.traffic = parseUserInfo(lower.value(QStringLiteral("subscription-userinfo")));
    meta.updateIntervalHours = parseInterval(lower.value(QStringLiteral("profile-update-interval")));
    return meta;
}

QList<VpnNode> parseSubscriptionBody(const QString &raw)
{
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};

    if (trimmed.startsWith(QLatin1Char('{')) || trimmed.startsWith(QLatin1Char('[')))
        fail(QStringLiteral("JSON-подписки пока не поддерживаются — "
                            "включите в панели обычный формат ссылок"));

    const QString text = trimmed.contains(QLatin1String("://"))
                             ? trimmed
                             : tryDecodeBase64(trimmed).value_or(QString());

    QList<VpnNode> nodes;
    QSet<QString> seen;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\r\n]+")),
                                        Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        const QString link = line.trimmed();
        if (link.isEmpty() || !link.contains(QLatin1String("://"))) continue;
        try {
            const auto node = parseLink(link);
            if (!node.has_value()) continue;
            if (!seen.contains(node->tag)) {
                seen.insert(node->tag);
                nodes.append(*node);
            }
        } catch (const std::invalid_argument &) {
            // битую строку пропускаем: остальные узлы важнее
        }
    }
    return nodes;
}
