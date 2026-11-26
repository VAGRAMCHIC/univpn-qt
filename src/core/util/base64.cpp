#include "base64.h"

#include <QByteArray>
#include <QRegularExpression>

std::optional<QString> tryDecodeBase64(const QString &value)
{
    QString compact = value;
    compact.remove(QRegularExpression(QStringLiteral("\\s")));
    if (compact.isEmpty())
        return std::nullopt;

    compact.replace(QLatin1Char('-'), QLatin1Char('+'));
    compact.replace(QLatin1Char('_'), QLatin1Char('/'));

    const int pad = (4 - compact.size() % 4) % 4;
    compact.append(QString(pad, QLatin1Char('=')));

    const QByteArray bytes = QByteArray::fromBase64(
        compact.toLatin1(), QByteArray::Base64Encoding);
    if (bytes.isEmpty() && !compact.isEmpty() && compact != QStringLiteral("===="))
        return std::nullopt;

    // Проверка: fromBase64 молча игнорирует невалидные символы,
    // поэтому перекодируем и сравниваем без паддинга.
    const QByteArray reencoded = bytes.toBase64();
    const QByteArray expected = compact.toLatin1();
    if (reencoded != expected && reencoded != expected.left(expected.size() - pad))
        return std::nullopt;

    return QString::fromUtf8(bytes);
}
