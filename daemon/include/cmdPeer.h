#ifndef CMD_PEER_H
#define CMD_PEER_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Peer Networking Command Handlers
CmdResult handleSetPeerConfig(const json &command);
CmdResult handleGetPeerStatus(const json &command);
CmdResult handleRegisterPeer(const json &command);
CmdResult handleListPeers(const json &command);
CmdResult handleDeletePeer(const json &command);
CmdResult handleGetPeerInfo(const json &command);
CmdResult handleExecOnPeer(const json &command);
CmdResult handleExecRequest(const json &command);
CmdResult handleRemotePull(const json &command);
CmdResult handleRemoteBd(const json &command);
CmdResult handleRemoteDeployDaemon(const json &command);
CmdResult handleDbSanityCheck(const json &command);
CmdResult handleRegisterWorker(const json &command);

#endif // CMD_PEER_H
