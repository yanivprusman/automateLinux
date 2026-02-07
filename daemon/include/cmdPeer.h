#ifndef CMD_PEER_H
#define CMD_PEER_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Shared utility: send command to daemon-manager.py on a remote peer
CmdResult sendToManager(const std::string &ip, const json &command);

// Shared utility: resolve peer IP from peer_id (works on leader or worker)
std::string resolvePeerIP(const std::string &peer_id);

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
CmdResult handleUpdatePeerMac(const json &command);
CmdResult handleUpdatePeerMacInternal(const json &command);

#endif // CMD_PEER_H
