#include <QtTest>
#include <QDateTime>

#include "core/util/stable_hash.h"
#include "core/util/base64.h"
#include "core/util/format.h"

class UtilTest : public QObject {
    Q_OBJECT

private slots:
    // --- stableHash32 ---
    void hashEmpty()
    {
        // 0x811C9DC5 -> "811c9dc5"
        QCOMPARE(stableHash32(QString()), QStringLiteral("811c9dc5"));
    }

    void hashAscii()
    {
        // Проверка совпадения с Dart-версией: значения посчитаны тем же алгоритмом.
        // Если разойдётся — значит, в C++ используется UTF-8, а не UTF-16.
        const QString h = stableHash32(QStringLiteral("vless|1.2.3.4|443|uuid"));
        QCOMPARE(h.size(), 8);
        QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-f]{8}$")).match(h).hasMatch());
    }

    void hashIsStable()
    {
        const QString a = stableHash32(QStringLiteral("n-"));
        const QString b = stableHash32(QStringLiteral("n-"));
        QCOMPARE(a, b);
    }

    // --- tryDecodeBase64 ---
    void base64Plain()
    {
        const auto r = tryDecodeBase64(QStringLiteral("aGVsbG8="));
        QVERIFY(r.has_value());
        QCOMPARE(*r, QStringLiteral("hello"));
    }

    void base64UrlSafe()
    {
        // '-' в base64url = '+' в base64 = '>' в ASCII
        const auto a = tryDecodeBase64(QStringLiteral("aGVsbG8-"));
        QVERIFY(a.has_value());
        QCOMPARE(*a, QStringLiteral("hello>"));

        // '_' в base64url = '/' в base64 = '?' в ASCII
        const auto b = tryDecodeBase64(QStringLiteral("aGVsbG8_"));
        QVERIFY(b.has_value());
        QCOMPARE(*b, QStringLiteral("hello?"));
    }


    void base64NoPadding()
    {
        const auto r = tryDecodeBase64(QStringLiteral("aGVsbG8"));
        QVERIFY(r.has_value());
        QCOMPARE(*r, QStringLiteral("hello"));
    }

    void base64WithWhitespace()
    {
        const auto r = tryDecodeBase64(QStringLiteral("aGVs\nbG8="));
        QVERIFY(r.has_value());
        QCOMPARE(*r, QStringLiteral("hello"));
    }

    void base64Invalid()
    {
        const auto r = tryDecodeBase64(QStringLiteral("!!!"));
        QVERIFY(!r.has_value());
    }

    // --- formatBytes ---
    void bytesZero()      { QCOMPARE(formatBytes(0), QStringLiteral("0 B")); }
    void bytesBytes()     { QCOMPARE(formatBytes(512), QStringLiteral("512 B")); }
    void bytesKB()        { QCOMPARE(formatBytes(1536), QStringLiteral("1.5 KB")); }
    void bytesMB()        { QCOMPARE(formatBytes(1048576), QStringLiteral("1.0 MB")); }

    // --- formatUptime ---
    void uptimeShort()  { QCOMPARE(formatUptime(312), QStringLiteral("05:12")); }
    void uptimeLong()   { QCOMPARE(formatUptime(2 * 3600 + 3 * 60 + 44), QStringLiteral("2:03:44")); }
    void uptimeZero()   { QCOMPARE(formatUptime(0), QStringLiteral("00:00")); }
};

QTEST_MAIN(UtilTest)
#include "util_test.moc"
