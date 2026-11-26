#pragma once

#include <QString>

/// Стабильный 32-битный FNV-1a хеш по UTF-16 code units.
/// Должен совпадать с Dart-версией (stable_hash.dart) бит-в-бит.
QString stableHash32(const QString &source);
