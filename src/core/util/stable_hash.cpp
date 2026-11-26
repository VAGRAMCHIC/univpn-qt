#include "stable_hash.h"

QString stableHash32(const QString &source)
{
    quint32 hash = 0x811C9DC5u;
    const auto *data = reinterpret_cast<const char16_t *>(source.utf16());
    const int length = source.size();

    for (int i = 0; i < length; ++i) {
        hash ^= static_cast<quint32>(data[i]);
        hash = (hash * 0x01000193u) & 0xFFFFFFFFu;
    }

    return QString::number(hash, 16).rightJustified(8, QLatin1Char('0'));
}
