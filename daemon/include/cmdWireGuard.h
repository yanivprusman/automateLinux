#ifndef CMD_WIREGUARD_H
#define CMD_WIREGUARD_H

#include "Types.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// WireGuard Setup Command Handlers
CmdResult handleSetupWireGuardPeer(const json &command);
CmdResult handleListWireGuardPeers(const json &command);
CmdResult handleGetWireGuardIp(const json &command);

#endif // CMD_WIREGUARD_H
