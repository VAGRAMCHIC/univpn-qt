#include "app_settings.h"

QJsonObject AppSettings::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("routingMode")] = routingModeToString(routingMode);
    o[QStringLiteral("autoConnect")] = autoConnect;
    if (localeCode.has_value())
        o[QStringLiteral("localeCode")] = *localeCode;
    if (selectedNodeTag.has_value())
        o[QStringLiteral("selectedNodeTag")] = *selectedNodeTag;
    o[QStringLiteral("killSwitch")] = killSwitch;
    return o;
}

AppSettings AppSettings::fromJson(const QJsonObject &json)
{
    AppSettings s;
    s.routingMode = routingModeFromString(
        json.value(QStringLiteral("routingMode")).toString());
    s.autoConnect = json.value(QStringLiteral("autoConnect")).toBool(false);

    if (json.contains(QStringLiteral("localeCode")))
        s.localeCode = json.value(QStringLiteral("localeCode")).toString();

    if (json.contains(QStringLiteral("selectedNodeTag")))
        s.selectedNodeTag = json.value(QStringLiteral("selectedNodeTag")).toString();

    s.killSwitch = json.value(QStringLiteral("killSwitch")).toBool(false);
    return s;
}
