#include "cmdPeer.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "PeerManager.h"
#include "Utils.h"
#include "Version.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

using namespace std;
using ordered_json = nlohmann::ordered_json;

// Forward declarations - implemented in DaemonServer.cpp
extern string getWgInterfaceIP();
extern string getPrimaryMacAddress();

// External client socket - set by mainCommand before calling handlers
extern int g_clientSocket;

static CmdResult sendToManager(const string &ip, const json &command) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return CmdResult(1, "Failed to create socket for manager\n");

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(MANAGER_PORT);
  if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
    close(sock);
    return CmdResult(1, "Invalid IP address for manager: " + ip + "\n");
  }

  // Set timeout
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  timeout.tv_sec = 300; // 5 minute timeout for build
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(sock);
    return CmdResult(1, "Failed to connect to manager at " + ip + ":" +
                            to_string(MANAGER_PORT) + "\n");
  }

  string msg = command.dump() + "\n";
  if (send(sock, msg.c_str(), msg.length(), 0) < 0) {
    close(sock);
    return CmdResult(1, "Failed to send command to manager at " + ip + "\n");
  }

  char buffer[65536];
  memset(buffer, 0, sizeof(buffer));
  ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
  close(sock);

  if (n <= 0)
    return CmdResult(1, "No response from manager at " + ip + "\n");

  try {
    json response = json::parse(buffer);
    int status =
        response.contains("status") ? response["status"].get<int>() : 1;
    string output = response.contains("output")
                        ? response["output"].get<string>()
                        : string(buffer);
    return CmdResult(status, output);
  } catch (const exception &e) {
    return CmdResult(0, string(buffer)); // Fallback to raw output
  }
}

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

  // If configured as leader, register self in database and start self-heartbeat
  if (pm.isLeader() && !pm.getPeerId().empty()) {
    string my_ip = getWgInterfaceIP();
    string my_mac = getPrimaryMacAddress();
    char hostname[256];
    string my_hostname = "";
    if (gethostname(hostname, sizeof(hostname)) == 0) {
      my_hostname = string(hostname);
    }
    PeerTable::upsertPeer(pm.getPeerId(), my_ip, my_mac, my_hostname, true,
                          DAEMON_VERSION);
    pm.startReconnectLoop();
  }

  // If configured as worker and leader address is set, try to connect
  if (pm.getRole() == PEER_ROLE_WORKER && !pm.getLeaderAddress().empty()) {
    if (pm.connectToLeader()) {
      return CmdResult(0, "Peer config saved. Connected to leader at " +
                              pm.getLeaderAddress() + "\n");
    } else {
      return CmdResult(0, "Peer config saved. Failed to connect to leader at " +
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
  int daemon_version = command.contains("daemon_version")
                           ? command["daemon_version"].get<int>()
                           : 0;

  // Store in database
  PeerTable::upsertPeer(peer_id, ip, mac, hostname, true, daemon_version);

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
  sort(peers.begin(), peers.end(),
       [](const PeerRecord &a, const PeerRecord &b) {
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

    // Derive is_online from last_seen freshness (heartbeat is every 60s)
    bool online = peer.is_online;
    if (online && !peer.last_seen.empty()) {
      // Parse "YYYY-MM-DD HH:MM:SS" and check if within 2 minutes
      struct tm tm = {};
      if (strptime(peer.last_seen.c_str(), "%Y-%m-%d %H:%M:%S", &tm)) {
        time_t lastSeen = mktime(&tm);
        time_t now = time(nullptr);
        if (difftime(now, lastSeen) > 120) {
          online = false;
        }
      }
    }
    p["is_online"] = online;
    p["daemon_version"] = peer.daemon_version;
    result.push_back(p);
  }

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleDeletePeer(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();

  PeerManager &pm = PeerManager::getInstance();

  // If we're a worker connected to leader, forward the request
  if (!pm.isLeader() && pm.isConnectedToLeader()) {
    json fwdCmd;
    fwdCmd["command"] = COMMAND_DELETE_PEER;
    fwdCmd[COMMAND_ARG_PEER] = peer_id;

    int leaderFd = pm.getLeaderSocket();
    string msg = fwdCmd.dump() + "\n";
    if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
      return CmdResult(1, "Failed to forward deletePeer to leader\n");
    }

    // Read response from leader
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(leaderFd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      return CmdResult(0, string(buffer));
    }
    return CmdResult(1, "No response from leader\n");
  }

  // Check if peer exists
  PeerRecord peer = PeerTable::getPeer(peer_id);
  if (peer.peer_id.empty()) {
    return CmdResult(1, "Peer not found: " + peer_id + "\n");
  }

  // Delete from database
  PeerTable::deletePeer(peer_id);

  logToFile("Peer deleted: " + peer_id, LOG_CORE);
  return CmdResult(0, "Deleted peer: " + peer_id + "\n");
}

CmdResult handleGetPeerInfo(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();

  PeerManager &pm = PeerManager::getInstance();

  // If we're a worker connected to leader, forward the request
  if (!pm.isLeader() && pm.isConnectedToLeader()) {
    json fwdCmd;
    fwdCmd["command"] = COMMAND_GET_PEER_INFO;
    fwdCmd[COMMAND_ARG_PEER] = peer_id;

    int leaderFd = pm.getLeaderSocket();
    string msg = fwdCmd.dump() + "\n";
    if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
      return CmdResult(1, "Failed to forward getPeerInfo to leader\n");
    }

    // Read response from leader
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(leaderFd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      return CmdResult(0, string(buffer));
    }
    return CmdResult(1, "No response from leader\n");
  }

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
  result["daemon_version"] = peer.daemon_version;

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleExecOnPeer(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();
  string directory = command[COMMAND_ARG_DIRECTORY].get<string>();
  string cmd = command[COMMAND_ARG_SHELL_CMD].get<string>();

  PeerManager &pm = PeerManager::getInstance();

  // Build exec request message
  json execRequest;
  execRequest["command"] = COMMAND_EXEC_REQUEST;
  execRequest[COMMAND_ARG_DIRECTORY] = directory;
  execRequest[COMMAND_ARG_SHELL_CMD] = cmd;

  for (int attempt = 0; attempt < 2; ++attempt) {
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
          return CmdResult(
              1, "Peer not found or leader unavailable: " + peer_id + "\n");
        }
      }

      logToFile("Peer " + peer_id +
                    " not connected, attempting on-demand connection to " + ip,
                LOG_CORE);

      if (!pm.connectToPeer(peer_id, ip)) {
        if (attempt == 0)
          continue;
        return CmdResult(1, "Failed to connect to peer: " + peer_id + " (" +
                                ip + ")\n");
      }

      // Re-fetch info after connection
      peer = pm.getPeerInfo(peer_id);
    }

    // Send to peer
    if (!pm.sendToPeer(peer_id, execRequest)) {
      if (attempt == 0) {
        logToFile("Send failed for " + peer_id + ", will retry...", LOG_CORE);
        continue; // pm.sendToPeer already cleared the stale connection
      }
      return CmdResult(1, "Failed to send command to peer: " + peer_id + "\n");
    }

    logToFile("Sent exec request to " + peer_id + ": cd " + directory + " && " +
                  cmd,
              LOG_CORE);

    // Set read timeout
    struct timeval timeout;
    timeout.tv_sec = 120; // 2 minute timeout for command execution
    timeout.tv_usec = 0;
    setsockopt(peer.socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
               sizeof(timeout));

    // Read response
    char buffer[65536];
    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(peer.socket_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
      // Clear from map if it was a connection error (not timeout)
      if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        pm.unregisterPeer(peer_id); // Clear stale connection
        if (attempt == 0) {
          logToFile("Read failed for " + peer_id + ", will retry...", LOG_CORE);
          continue;
        }
      }

      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return CmdResult(1,
                         "Timeout waiting for response from " + peer_id + "\n");
      }
      return CmdResult(1, "Failed to read response from " + peer_id + ": " +
                              strerror(errno) + "\n");
    }

    return CmdResult(0, string(buffer));
  }

  return CmdResult(1, "Unknown error in handleExecOnPeer\n");
}

// Execute a command in a forked process to avoid blocking the daemon
// Returns output via pipe, with timeout protection
static string executeCommandWithTimeout(const string &full_cmd, int timeout_sec,
                                        int &exit_code) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    exit_code = 1;
    return "Failed to create pipe: " + string(strerror(errno));
  }

  pid_t pid = fork();
  if (pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    exit_code = 1;
    return "Failed to fork: " + string(strerror(errno));
  }

  if (pid == 0) {
    // Child process
    close(pipefd[0]); // Close read end
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    // Execute via shell
    execl("/bin/sh", "sh", "-c", full_cmd.c_str(), nullptr);
    _exit(127); // exec failed
  }

  // Parent process
  close(pipefd[1]); // Close write end

  // Set read timeout using select
  string output;
  char buffer[256];
  fd_set readfds;
  struct timeval tv;
  time_t start_time = time(nullptr);

  while (true) {
    FD_ZERO(&readfds);
    FD_SET(pipefd[0], &readfds);

    // Calculate remaining timeout
    int elapsed = time(nullptr) - start_time;
    int remaining = timeout_sec - elapsed;
    if (remaining <= 0) {
      // Timeout - kill the child
      kill(pid, SIGKILL);
      waitpid(pid, nullptr, 0);
      close(pipefd[0]);
      exit_code = 124; // timeout exit code
      return output + "\n[TIMEOUT after " + to_string(timeout_sec) + "s]";
    }

    tv.tv_sec = remaining;
    tv.tv_usec = 0;

    int ret = select(pipefd[0] + 1, &readfds, nullptr, nullptr, &tv);
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      break;
    }
    if (ret == 0) {
      // Timeout
      kill(pid, SIGKILL);
      waitpid(pid, nullptr, 0);
      close(pipefd[0]);
      exit_code = 124;
      return output + "\n[TIMEOUT after " + to_string(timeout_sec) + "s]";
    }

    ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
    if (n <= 0)
      break;
    buffer[n] = '\0';
    output += buffer;
  }

  close(pipefd[0]);

  // Wait for child and get exit code
  int status;
  waitpid(pid, &status, 0);
  exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

  return output;
}

CmdResult handleExecRequest(const json &command) {
  string directory = command[COMMAND_ARG_DIRECTORY].get<string>();
  string cmd = command[COMMAND_ARG_SHELL_CMD].get<string>();

  // Prevent deadlock: reject commands that would try to connect back to this
  // daemon
  if (cmd.find("daemon send") != string::npos ||
      cmd.find("daemon daemon") != string::npos ||
      cmd.find("/daemon send") != string::npos ||
      cmd.find(" d ") != string::npos ||
      (cmd.length() >= 2 && cmd[0] == 'd' && cmd[1] == ' ')) {
    logToFile("Rejected deadlock-prone command: " + cmd, LOG_CORE);
    return CmdResult(1, "Error: Cannot run daemon commands via execOnPeer "
                        "(would cause deadlock). "
                        "Use direct daemon commands instead (e.g., 'd "
                        "listPorts' locally forwards to leader).\n");
  }

  // Build full command: cd to directory && execute command
  string full_cmd = "cd " + directory + " && " + cmd + " 2>&1";

  logToFile("Executing: " + full_cmd, LOG_CORE);

  // Execute with 60 second timeout to prevent hangs
  int exit_code = 0;
  string output = executeCommandWithTimeout(full_cmd, 60, exit_code);

  logToFile("Command completed with exit code " + to_string(exit_code),
            LOG_CORE);

  return CmdResult(exit_code, output);
}

CmdResult handleRemotePull(const json &command) {
  json execCmd;
  execCmd["command"] = COMMAND_EXEC_ON_PEER;
  execCmd[COMMAND_ARG_PEER] = command[COMMAND_ARG_PEER].get<string>();
  execCmd[COMMAND_ARG_DIRECTORY] = "/opt/automateLinux";
  execCmd[COMMAND_ARG_SHELL_CMD] = "git pull";
  return handleExecOnPeer(execCmd);
}

CmdResult handleRemoteBd(const json &command) {
  json execCmd;
  execCmd["command"] = COMMAND_EXEC_ON_PEER;
  execCmd[COMMAND_ARG_PEER] = command[COMMAND_ARG_PEER].get<string>();
  execCmd[COMMAND_ARG_DIRECTORY] = "/opt/automateLinux/daemon";
  execCmd[COMMAND_ARG_SHELL_CMD] = "bash -c 'source ./build.sh'";
  return handleExecOnPeer(execCmd);
}

CmdResult handleRemoteDeployDaemon(const json &command) {
  string peer_id = command[COMMAND_ARG_PEER].get<string>();
  PeerManager &pm = PeerManager::getInstance();

  // Find IP for this peer
  string ip;
  if (pm.isLeader()) {
    PeerRecord dbPeer = PeerTable::getPeer(peer_id);
    ip = dbPeer.ip_address;
    if (ip.empty() && peer_id == pm.getPeerId())
      ip = "127.0.0.1";
  } else {
    if (peer_id == pm.getPeerId()) {
      ip = "127.0.0.1";
    } else {
      ip = queryLeaderForPeerIP(peer_id);
    }
  }

  if (ip.empty()) {
    return CmdResult(1, "Could not find IP for peer: " + peer_id + "\n");
  }

  logToFile("Directing deploy request to manager at " + ip, LOG_CORE);

  // Send single 'deploy' command to manager
  json mgrCmd;
  mgrCmd["command"] = "deploy";

  return sendToManager(ip, mgrCmd);
}

CmdResult handleDbSanityCheck(const json &) {
  PeerManager &pm = PeerManager::getInstance();
  string role = pm.getRole();
  ordered_json result;
  result["role"] = role;
  result["issues_found"] = false;
  result["actions"] = ordered_json::array();

  // Only workers need sanity checks - leaders are supposed to have peer data
  if (role != PEER_ROLE_WORKER) {
    result["message"] = "No sanity check needed for role: " + role;
    return CmdResult(0, result.dump(2) + "\n");
  }

  // Check for peer_registry data on a worker (should not exist)
  auto peers = PeerTable::getAllPeers();
  if (!peers.empty()) {
    result["issues_found"] = true;
    ordered_json action;
    action["issue"] = "Worker has peer_registry data (leader-only table)";
    action["peer_count"] = (int)peers.size();

    // List the errant peer IDs
    ordered_json peer_ids = ordered_json::array();
    for (const auto &peer : peers) {
      peer_ids.push_back(peer.peer_id);
    }
    action["peer_ids"] = peer_ids;

    // Clear the table
    int deleted = PeerTable::clearAllPeers();
    action["action_taken"] = "Deleted " + to_string(deleted) + " peer records";
    result["actions"].push_back(action);
  }

  if (result["issues_found"].get<bool>()) {
    result["message"] = "Sanity check completed: issues found and fixed";
  } else {
    result["message"] = "Sanity check passed: no issues found";
  }

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleRegisterWorker(const json &) {
  PeerManager &pm = PeerManager::getInstance();

  // Get hostname for peer_id
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    return CmdResult(1, "Failed to get hostname\n");
  }
  string peer_id(hostname);

  // Configure as worker
  pm.setRole(PEER_ROLE_WORKER);
  pm.setPeerId(peer_id);
  pm.setLeaderAddress(WG_LEADER_IP);

  // Connect to leader
  if (pm.connectToLeader()) {
    return CmdResult(0, "Registered as worker '" + peer_id +
                            "', connected to leader at " +
                            string(WG_LEADER_IP) + "\n");
  } else {
    return CmdResult(0, "Registered as worker '" + peer_id +
                            "', failed to connect to leader at " +
                            string(WG_LEADER_IP) +
                            " (will retry on next command)\n");
  }
}

CmdResult handleUpdatePeerMac(const json &) {
  PeerManager &pm = PeerManager::getInstance();
  string peer_id = pm.getPeerId();

  if (peer_id.empty()) {
    return CmdResult(1, "Peer not configured. Run registerWorker first.\n");
  }

  string mac = getPrimaryMacAddress();
  if (mac.empty()) {
    return CmdResult(1, "Failed to detect MAC address\n");
  }

  // If we're the leader, update directly in database
  if (pm.isLeader()) {
    PeerRecord peer = PeerTable::getPeer(peer_id);
    if (peer.peer_id.empty()) {
      return CmdResult(1, "Peer not found in database: " + peer_id + "\n");
    }
    PeerTable::upsertPeer(peer_id, peer.ip_address, mac, peer.hostname,
                          peer.is_online, peer.daemon_version);
    return CmdResult(0, "Updated MAC for " + peer_id + ": " + mac + "\n");
  }

  // Worker: send update to leader
  if (!pm.isConnectedToLeader()) {
    // Try to connect
    if (!pm.connectToLeader()) {
      return CmdResult(1, "Not connected to leader\n");
    }
  }

  json updateMsg;
  updateMsg["command"] = "updatePeerMacInternal";
  updateMsg["peer_id"] = peer_id;
  updateMsg["mac"] = mac;

  if (!pm.sendToLeader(updateMsg)) {
    return CmdResult(1, "Failed to send MAC update to leader\n");
  }

  // Read response
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int leaderFd = pm.getLeaderSocket();
  ssize_t n = read(leaderFd, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    return CmdResult(0, string(buffer));
  }

  return CmdResult(0, "Sent MAC update to leader: " + mac + "\n");
}

CmdResult handleUpdatePeerMacInternal(const json &command) {
  // This is called on the leader when a worker sends their MAC update
  PeerManager &pm = PeerManager::getInstance();

  if (!pm.isLeader()) {
    return CmdResult(1, "This command should only be received by the leader\n");
  }

  string peer_id = command["peer_id"].get<string>();
  string mac = command["mac"].get<string>();

  PeerRecord peer = PeerTable::getPeer(peer_id);
  if (peer.peer_id.empty()) {
    return CmdResult(1, "Peer not found: " + peer_id + "\n");
  }

  PeerTable::upsertPeer(peer_id, peer.ip_address, mac, peer.hostname,
                        peer.is_online, peer.daemon_version);
  logToFile("Updated MAC for peer " + peer_id + ": " + mac, LOG_CORE);
  return CmdResult(0, "Updated MAC for " + peer_id + ": " + mac + "\n");
}
