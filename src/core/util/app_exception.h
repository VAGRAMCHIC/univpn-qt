#pragma once

#include <stdexcept>
#include <QString>

/// Базовое исключение приложения: сообщение уже готово для показа пользователю.
class AppException : public std::runtime_error {
public:
    explicit AppException(const QString &message)
        : std::runtime_error(message.toStdString())
        , m_message(message) {}

    QString message() const { return m_message; }

private:
    QString m_message;
};
