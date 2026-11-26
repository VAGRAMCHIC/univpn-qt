#pragma once

#include <QString>
#include "../settings/app_settings.h"

/// Настройки в JSON-файле рядом с подписками.
class SettingsStore {
public:
    explicit SettingsStore(QString filePath);

    AppSettings load() const;
    bool save(const AppSettings &settings) const;

    QString filePath() const { return m_filePath; }

private:
    QString m_filePath;
};
