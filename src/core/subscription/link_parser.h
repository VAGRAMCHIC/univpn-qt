#pragma once

#include <QString>
#include <optional>
#include "../model/vpn_node.h"

/// Разбор пользовательских ссылок вида vless://…, trojan://…, ss://…
///
/// Возвращает nullopt для неизвестных схем.
/// Для известных, но битых ссылок — выбрасывает std::runtime_error
/// с сообщением, пригодным для показа пользователю.
std::optional<VpnNode> parseLink(const QString &rawLink);
