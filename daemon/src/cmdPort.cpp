#include "cmdPort.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "PeerManager.h"
#include "Utils.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

// Helper to forward a command to the leader and return the response
static CmdResult forwardToLeader(const json &cmd, const string &cmdName) {
  PeerManager &pm = PeerManager::getInstance();

  logToFile("forwardToLeader: " + cmdName + " isLeader=" +
            (pm.isLeader() ? "true" : "false") +
            " connectedToLeader=" + (pm.isConnectedToLeader() ? "true" : "false"), LOG_CORE);

  if (pm.isLeader()) {
    // We are the leader, don't forward
    logToFile("forwardToLeader: handling locally (we are leader)", LOG_CORE);
    return CmdResult(-1, ""); // Signal to handle locally
  }

  if (!pm.isConnectedToLeader()) {
    // Try to connect
    logToFile("forwardToLeader: not connected, trying to connect", LOG_CORE);
    if (!pm.connectToLeader()) {
      return CmdResult(1, "Not connected to leader. Run 'd registerWorker' first.\n");
    }
  }

  int leaderFd = pm.getLeaderSocket();
  string msg = cmd.dump() + "\n";
  if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
    return CmdResult(1, "Failed to forward " + cmdName + " to leader\n");
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

static std::string getGitVersion(const std::string &path) {
  std::string cmdHash = "git -C " + path + " rev-parse --short HEAD";
  std::string hash = executeCommand(cmdHash.c_str());

  if (hash.empty()) {
    return "N/A";
  }

  // Clean up newlines
  if (!hash.empty() && hash.back() == '\n')
    hash.pop_back();

  std::string cmdMsg = "git -C " + path + " log -1 --format=\"%s\"";
  std::string msg = executeCommand(cmdMsg.c_str());
  if (!msg.empty() && msg.back() == '\n')
    msg.pop_back();

  return hash + " - " + msg;
}

CmdResult handleGetPort(const json &command) {
  // Forward to leader if we're a worker
  json fwdCmd = command;
  fwdCmd["command"] = COMMAND_GET_PORT;
  CmdResult fwdResult = forwardToLeader(fwdCmd, "getPort");
  if (fwdResult.status != -1) {
    return fwdResult; // Forwarded successfully or error
  }

  // We are the leader, handle locally
  string key = command[COMMAND_ARG_KEY].get<string>();
  string portKey = "port_" + key;
  string value = SettingsTable::getSetting(portKey);
  if (value.empty()) {
    return CmdResult(1, "Port not set for " + key + "\n");
  }
  return CmdResult(0, value + "\n");
}

CmdResult handleSetPort(const json &command) {
  // Forward to leader if we're a worker
  json fwdCmd = command;
  fwdCmd["command"] = COMMAND_SET_PORT;
  CmdResult fwdResult = forwardToLeader(fwdCmd, "setPort");
  if (fwdResult.status != -1) {
    return fwdResult; // Forwarded successfully or error
  }

  // We are the leader, handle locally
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value;
  if (command[COMMAND_ARG_VALUE].is_number()) {
    value = to_string(command[COMMAND_ARG_VALUE].get<long long>());
  } else {
    value = command[COMMAND_ARG_VALUE].get<string>();
  }
  string portKey = "port_" + key;
  SettingsTable::setSetting(portKey, value);
  return CmdResult(0, "Port set for " + key + " to " + value + "\n");
}

CmdResult handleDeletePort(const json &command) {
  // Forward to leader if we're a worker
  json fwdCmd = command;
  fwdCmd["command"] = COMMAND_DELETE_PORT;
  CmdResult fwdResult = forwardToLeader(fwdCmd, "deletePort");
  if (fwdResult.status != -1) {
    return fwdResult; // Forwarded successfully or error
  }

  // We are the leader, handle locally
  string key = command[COMMAND_ARG_KEY].get<string>();
  string portKey = "port_" + key;
  int rc = SettingsTable::deleteSetting(portKey);
  if (rc >= 1) {
    return CmdResult(0, "Port entry deleted for " + key + "\n");
  }
  return CmdResult(1, "Port entry not found for " + key + "\n");
}

CmdResult handleListPorts(const json &command) {
  // Forward to leader if we're a worker
  json fwdCmd;
  fwdCmd["command"] = COMMAND_LIST_PORTS;
  CmdResult fwdResult = forwardToLeader(fwdCmd, "listPorts");
  if (fwdResult.status != -1) {
    return fwdResult; // Forwarded successfully or error
  }

  // We are the leader, handle locally
  auto allSettings = SettingsTable::getAllSettings();
  std::vector<std::pair<std::string, int>> ports;

  for (const auto &p : allSettings) {
    if (p.first.find("port_") == 0) {
      try {
        ports.push_back({p.first.substr(5), std::stoi(p.second)});
      } catch (...) {
        // Skip invalid ports
      }
    }
  }

  // Sort by port number (second element of pair)
  std::sort(ports.begin(), ports.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });

  std::stringstream ss;
  ss << "--- Registered Port Mappings ---\n";
  ss << std::left << std::setw(20) << "Key" << std::setw(8) << "Port"
     << "Version" << "\n";
  ss << std::string(80, '-') << "\n";

  if (ports.empty()) {
    ss << "  (No ports registered)\n";
  } else {
    for (const auto &p : ports) {
      std::string key = p.first;
      int port = p.second;
      std::string repoPath = "";

      // Determine repository path based on key
      if (key == "dashboard-dev" || key == "dashboard-prod" ||
          key == "dashboard-bridge") {
        repoPath = directories.base;
      } else if (key == "cad-dev") {
        repoPath = directories.base + "extraApps/cad";
      } else if (key == "cad-prod") {
        repoPath = "/opt/prod/cad";
      } else if (key == "pt-dev") {
        repoPath = directories.base + "extraApps/publicTransportation";
      } else if (key == "pt-prod") {
        repoPath = "/opt/prod/publicTransportation";
      }

      std::string versionInfo = "";
      if (!repoPath.empty()) {
        versionInfo = getGitVersion(repoPath);
      }

      ss << std::left << std::setw(20) << key << std::setw(8) << port
         << versionInfo << "\n";
    }
  }

  return CmdResult(0, ss.str());
}

CmdResult handlePublicTransportationStartProxy(const json &) {
  // 1. Get port from registry
  string portStr = SettingsTable::getSetting("port_pt-proxy");
  int port = 0;

  if (!portStr.empty()) {
    try {
      port = std::stoi(portStr);
    } catch (...) {
      return CmdResult(1, "Invalid port for pt-proxy in registry: " + portStr +
                              "\n");
    }
  }

  // 2. If not found in registry, find a free port and save it
  if (port == 0) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      return CmdResult(1, "Failed to create socket for port discovery\n");
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = 0; // Let OS choose

    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      close(sock);
      return CmdResult(1, "Failed to bind to find free port\n");
    }

    socklen_t len = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &len) < 0) {
      close(sock);
      return CmdResult(1, "Failed to get socket name\n");
    }

    port = ntohs(sin.sin_port);
    close(sock);

    // Save to registry (automatically prepends 'port_')
    SettingsTable::setSetting("port_pt-proxy", to_string(port));
  }

  // 3. Start SSH Tunnel
  string cmd =
      "ssh -f -N -L " + to_string(port) + ":moran.mot.gov.il:110 root@10.0.0.1";
  int rc = std::system(cmd.c_str());

  if (rc != 0) {
    return CmdResult(1, "Failed to start SSH tunnel\n");
  }

  // 4. Write port to temp file (for legacy compatibility with PHP apps)
  ofstream portFile("/tmp/pt_proxy_port");
  if (portFile.is_open()) {
    portFile << port;
    portFile.close();
    chmod("/tmp/pt_proxy_port", 0644);
  } else {
    return CmdResult(1, "Failed to write port file\n");
  }

  return CmdResult(0, "Proxy started on port " + to_string(port) + "\n");
}

CmdResult handlePublicTransportationOpenApp(const json &command) {
  // Query the daemon's port registry for the PT app port
  string portKey = "port_pt";
  string port = "";

  // 1. Try to use explicit variant if provided
  if (command.contains("variant")) {
    string variant = command["variant"].get<string>();
    portKey = "port_pt-" + variant;
    port = SettingsTable::getSetting(portKey);
    if (port.empty()) {
      return CmdResult(1, "PT app port for variant '" + variant +
                              "' not found.\n");
    }
  } else {
    // 2. Fallback sequence: pt -> pt-prod -> pt-dev
    port = SettingsTable::getSetting("port_pt");
    if (port.empty()) {
      port = SettingsTable::getSetting("port_pt-prod");
      if (port.empty()) {
        port = SettingsTable::getSetting("port_pt-dev");
      }
    }
  }

  if (port.empty()) {
    return CmdResult(
        1, "PT app port not set. Use 'd setPort pt <PORT>' to configure.\n");
  }

  string url = "http://localhost:" + port;
  string cmd = "google-chrome " + url + " > /dev/null 2>&1 &";
  int rc = std::system(cmd.c_str());

  if (rc != 0) {
    return CmdResult(1, "Failed to open Chrome\n");
  }

  return CmdResult(0, "Opened " + url + " in Chrome\n");
}
