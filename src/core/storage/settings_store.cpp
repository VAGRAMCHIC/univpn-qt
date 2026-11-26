#include "settings_store.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

SettingsStore::SettingsStore(QString filePath)
    : m_filePath(std::move(filePath)) {}

AppSettings SettingsStore::load() const
{
    QFile file(m_filePath);
    if (!file.exists())
        return AppSettings{};

    if (!file.open(QIODevice::ReadOnly))
        return AppSettings{};

    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return AppSettings{};

    return AppSettings::fromJson(doc.object());
}

bool SettingsStore::save(const AppSettings &settings) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    const QJsonDocument doc(settings.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}
