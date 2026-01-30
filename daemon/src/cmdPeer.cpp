#include "cmdPeer.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "PeerManager.h"
#include "Utils.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using ordered_json = nlohmann::ordered_json;

// Forward declaration - implemented in DaemonServer.cpp
extern string getWgInterfaceIP();

// External client socket - set by mainCommand before calling handlers
extern int g_clientSocket;

// Helper function for workers to query leader for peer IP
static string queryLeaderForPeerIP(const string &peer_id) {
  PeerManager &pm = PeerManager::getInstance();

  if (!pm.isConnectedToLeader()) {
    logToFile("Cannot query leader: not connected", LOG_CORE);
    return "";
  }

  json query;
  query["command"] = COMMAND_GET_PEER_INFO;
  query[COMMAND_ARG_PEER] = peer_id;

  if (!pm.sendToLeader(query)) {
    logToFile("Failed to send getPeerInfo to leader", LOG_CORE);
    return "";
  }

  // Read response from leader
  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  int leaderFd = pm.getLeaderSocket();
  ssize_t n = read(leaderFd, buffer, sizeof(buffer) - 1);
  if (n <= 0) {
    logToFile("No response from leader for getPeerInfo", LOG_CORE);
    return "";
  }

  try {
    json response = json::parse(buffer);
    if (response.contains("ip_address")) {
      return response["ip_address"].get<string>();
    }
  } catch (const exception &e) {
    logToFile("Failed to parse leader response: " + string(e.what()), LOG_CORE);
  }

  return "";
}

CmdResult handleSetPeerConfig(const json &command) {
  PeerManager &pm = PeerManager::getInstance();

  if (command.contains(COMMAND_ARG_ROLE)) {
    string role = command[COMMAND_ARG_ROLE].get<string>();
    if (role != PEER_ROLE_LEADER && role != PEER_ROLE_WORKER) {
      return CmdResult(1, "Invalid role. Use 'leader' or 'worker'.\n");
    }
    pm.setRole(role);
  }

  if (command.contains(COMMAND_ARG_ID)) {
    pm.setPeerId(command[COMMAND_ARG_ID].get<string>());
  }

  if (command.contains(COMMAND_ARG_LEADER)) {
    pm.setLeaderAddress(command[COMMAND_ARG_LEADER].get<string>());
  }

  // If configured as leader, register self in database
  if (pm.isLeader() && !pm.getPeerId().empty()) {
    string my_ip = getWgInterfaceIP();
    char hostname[256];
    string my_hostname = "";
    if (gethostname(hostname, sizeof(hostname)) == 0) {
      my_hostname = string(hostname);
    }
    PeerTable::upsertPeer(pm.getPeerId(), my_ip, "", my_hostname, true);
  }

  // If configured as worker and leader address is set, try to connect
  if (pm.getRole() == PEER_ROLE_WORKER && !pm.getLeaderAddress().empty()) {
    if (pm.connectToLeader()) {
      return CmdResult(0, "Peer config saved. Connected to leader at " +
                              pm.getLeaderAddress() + "\n");
    } else {
      return CmdResult(0,
                       "Peer config saved. Failed to connect to leader at " +
                           pm.getLeaderAddress() +
                           " (will retry on next command)\n");
    }
  }

  return CmdResult(0, "Peer config saved. Role: " + pm.getRole() +
                          ", ID: " + pm.getPeerId() + "\n");
}

CmdResult handleGetPeerStatus(const json &) {
  PeerManager &pm = PeerManager::getInstance();
  ordered_json status;
  status["role"] = pm.getRole();
  status["peer_id"] = pm.getPeerId();
  status["leader_address"] = pm.getLeaderAddress();
  status["is_leader"] = pm.isLeader();
  status["connected_to_leader"] = pm.isConnectedToLeader();

  if (pm.isLeader()) {
    auto peers = pm.listPeers();
    status["connected_workers"] = (int)peers.size();
  }

  return CmdResult(0, status.dump(2) + "\n");
}

CmdResult handleRegisterPeer(const json &command) {
  // This is called by a worker connecting to the leader
  PeerManager &pm = PeerManager::getInstance();

  if (!pm.isLeader()) {
    return CmdResult(1, "This daemon is not the leader.\n");
  }

  string peer_id = command.contains("peer_id")
                       ? command["peer_id"].get<string>()
                       : "unknown";
  string ip = command.contains("ip") ? command["ip"].get<string>() : "";
  string mac = command.contains("mac") ? command["mac"].get<string>() : "";
  string hostname =
      command.contains("hostname") ? command["hostname"].get<string>() : "";

  // Store in database
  PeerTable::upsertPeer(peer_id, ip, mac, hostname, true);

  // Register in memory
  pm.registerPeer(peer_id, ip, mac, hostname, g_clientSocket);

  logToFile("Peer registered: " + peer_id + " (" + ip + ")", LOG_CORE);
  return CmdResult(0, "Peer registered: " + peer_id + "\n");
}

CmdResult handleListPeers(const json &) {
  PeerManager &pm = PeerManager::getInstance();

  // If we're a worker connected to leader, forward the request
  if (!pm.isLeader() && pm.isConnectedToLeader()) {
    json fwdCmd;
    fwdCmd["command"] = COMMAND_LIST_PEERS;

    int leaderFd = pm.getLeaderSocket();
    string msg = fwdCmd.dump() + "\n";
    if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
      return CmdResult(1, "Failed to forward listPeers to leader\n");
    }

    // Read response from leader
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(leaderFd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      return CmdResult(0, string(buffer));
    }
    return CmdResult(1, "No response from leader\n");
  }

  auto peers = PeerTable::getAllPeers();

  // Sort by IP address (numeric comparison of last octet for 10.0.0.x)
  sort(peers.begin(), peers.end(), [](const PeerRecord &a, const PeerRecord &b) {
    auto getLastOctet = [](const string &ip) -> int {
      size_t lastDot = ip.rfind('.');
      if (lastDot != string::npos) {
        return stoi(ip.substr(lastDot + 1));
      }
      return 0;
    };
    return getLastOctet(a.ip_address) < getLastOctet(b.ip_address);
  });

  ordered_json result = ordered_json::array();

  for (const auto &peer : peers) {
    ordered_json p;
    p["peer_id"] = peer.peer_id;
    p["ip_address"] = peer.ip_address;
    p["mac_address"] = peer.mac_address;
    p["hostname"] = peer.hostname;
    p["last_seen"] = peer.last_seen;
    p["is_online"] = peer.is_online;
    result.push_back(p);
  }

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleGetPeerInfo(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();
  PeerRecord peer = PeerTable::getPeer(peer_id);

  if (peer.peer_id.empty()) {
    return CmdResult(1, "Peer not found: " + peer_id + "\n");
  }

  ordered_json result;
  result["peer_id"] = peer.peer_id;
  result["ip_address"] = peer.ip_address;
  result["mac_address"] = peer.mac_address;
  result["hostname"] = peer.hostname;
  result["last_seen"] = peer.last_seen;
  result["is_online"] = peer.is_online;

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleExecOnPeer(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();
  string directory = command[COMMAND_ARG_DIRECTORY].get<string>();
  string cmd = command[COMMAND_ARG_SHELL_CMD].get<string>();

  PeerManager &pm = PeerManager::getInstance();

  // Check if peer is already connected in memory
  PeerInfo peer = pm.getPeerInfo(peer_id);
  if (peer.peer_id.empty() || peer.socket_fd < 0) {
    // Not connected - need to find IP and connect on-demand
    string ip;

    if (pm.isLeader()) {
      // Leader: look up in local database
      PeerRecord dbPeer = PeerTable::getPeer(peer_id);
      if (dbPeer.peer_id.empty()) {
        return CmdResult(1, "Peer not found: " + peer_id + "\n");
      }
      if (dbPeer.ip_address.empty()) {
        return CmdResult(1, "Peer has no IP address: " + peer_id + "\n");
      }
      ip = dbPeer.ip_address;
    } else {
      // Worker: query leader for peer IP
      if (!pm.isConnectedToLeader()) {
        return CmdResult(1, "Not connected to leader\n");
      }
      ip = queryLeaderForPeerIP(peer_id);
      if (ip.empty()) {
        return CmdResult(1, "Peer not found or leader unavailable: " + peer_id + "\n");
      }
    }

    logToFile("Peer " + peer_id + " not connected, attempting on-demand connection to " + ip,
              LOG_CORE);

    if (!pm.connectToPeer(peer_id, ip)) {
      return CmdResult(1, "Failed to connect to peer: " + peer_id + " (" + ip + ")\n");
    }
  }

  // Build exec request message
  json execRequest;
  execRequest["command"] = COMMAND_EXEC_REQUEST;
  execRequest[COMMAND_ARG_DIRECTORY] = directory;
  execRequest[COMMAND_ARG_SHELL_CMD] = cmd;

  // Send to peer
  if (!pm.sendToPeer(peer_id, execRequest)) {
    return CmdResult(1, "Failed to send command to peer: " + peer_id + "\n");
  }

  logToFile("Sent exec request to " + peer_id + ": cd " + directory + " && " + cmd, LOG_CORE);
  return CmdResult(0, "Exec request sent to " + peer_id + "\n");
}

CmdResult handleExecRequest(const json &command) {
  string directory = command[COMMAND_ARG_DIRECTORY].get<string>();
  string cmd = command[COMMAND_ARG_SHELL_CMD].get<string>();

  // Build full command: cd to directory && execute command
  string full_cmd = "cd " + directory + " && " + cmd + " 2>&1";

  logToFile("Executing: " + full_cmd, LOG_CORE);

  // Execute command and capture output
  FILE *pipe = popen(full_cmd.c_str(), "r");
  if (!pipe) {
    string error_msg = "Failed to execute command: " + string(strerror(errno));
    logToFile(error_msg, LOG_CORE);
    return CmdResult(1, error_msg + "\n");
  }

  // Read output
  string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

  int exit_code = pclose(pipe);
  int actual_exit_code = WEXITSTATUS(exit_code);

  logToFile("Command completed with exit code " + to_string(actual_exit_code), LOG_CORE);

  if (actual_exit_code != 0) {
    return CmdResult(actual_exit_code, output);
  }

  return CmdResult(0, output);
}
