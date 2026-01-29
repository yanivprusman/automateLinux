#include "cmdWireGuard.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include <cstring>

using namespace std;
using ordered_json = nlohmann::ordered_json;

CmdResult handleSetupWireGuardPeer(const json &command) {
  string name = command[COMMAND_ARG_NAME].get<string>();
  string host = command.value(COMMAND_ARG_HOST, "");
  string vpnIp = command.value(COMMAND_ARG_VPN_IP, "");
  string mac = command.value(COMMAND_ARG_MAC, "");
  bool dualBoot = command.value(COMMAND_ARG_DUAL_BOOT, false);
  string privateKey = command.value(COMMAND_ARG_PRIVATE_KEY, "");

  // Validate name
  if (name.empty()) {
    return CmdResult(1, "Name is required (--name)\n");
  }

  // Build script path and arguments
  string scriptPath = directories.base + "daemon/scripts/setup_wireguard_peer.sh";
  string args = "--name " + name;

  if (!vpnIp.empty()) {
    args += " --ip " + vpnIp;
  }
  if (!mac.empty()) {
    args += " --mac " + mac;
  }
  if (dualBoot) {
    args += " --dual-boot";
  }
  if (!privateKey.empty()) {
    args += " --private-key '" + privateKey + "'";
  }
  // Always pass leader IP for daemon registration
  args += " --leader-ip " + string(WG_LEADER_IP);

  string full_cmd;
  if (host.empty()) {
    // Run locally
    full_cmd = "bash " + scriptPath + " " + args + " 2>&1";
    logToFile("Setting up WireGuard locally: " + full_cmd, LOG_CORE);
  } else {
    // Run remotely via SSH
    // Copy script to remote and execute
    full_cmd = "ssh -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10 root@" +
               host + " 'bash -s -- " + args + "' < " + scriptPath + " 2>&1";
    logToFile("Setting up WireGuard on remote host " + host + ": " + args, LOG_CORE);
  }

  // Execute command and capture output
  FILE *pipe = popen(full_cmd.c_str(), "r");
  if (!pipe) {
    string error_msg = "Failed to execute setup script: " + string(strerror(errno));
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

  logToFile("WireGuard setup completed with exit code " + to_string(actual_exit_code), LOG_CORE);

  if (actual_exit_code != 0) {
    return CmdResult(actual_exit_code, output);
  }

  return CmdResult(0, output);
}

CmdResult handleListWireGuardPeers(const json &) {
  // SSH to the WireGuard server and list peers from wg0.conf
  string cmd = "ssh -o StrictHostKeyChecking=accept-new -o ConnectTimeout=5 " +
               string(WG_SERVER_USER) + "@" + string(WG_SERVER_IP) +
               " 'grep -E \"^\\[Peer\\]|^# |^PublicKey|^AllowedIPs\" /etc/wireguard/wg0.conf' 2>&1";

  logToFile("Listing WireGuard peers from VPS", LOG_CORE);

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return CmdResult(1, "Failed to connect to WireGuard server\n");
  }

  string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

  int exit_code = pclose(pipe);
  int actual_exit_code = WEXITSTATUS(exit_code);

  if (actual_exit_code != 0) {
    return CmdResult(actual_exit_code, "Failed to list peers: " + output);
  }

  // Parse the output into structured format
  ordered_json peers = ordered_json::array();
  ordered_json current_peer;

  istringstream stream(output);
  string line;
  while (getline(stream, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t");
    if (start == string::npos) continue;
    line = line.substr(start);

    if (line.find("[Peer]") == 0) {
      // Save previous peer if exists
      if (!current_peer.empty()) {
        peers.push_back(current_peer);
      }
      current_peer = ordered_json();
    } else if (line[0] == '#' && line.length() > 2) {
      // Comment line contains peer name
      current_peer["name"] = line.substr(2);
    } else if (line.find("PublicKey") == 0) {
      size_t eq = line.find('=');
      if (eq != string::npos) {
        string value = line.substr(eq + 1);
        size_t vstart = value.find_first_not_of(" ");
        if (vstart != string::npos) {
          current_peer["public_key"] = value.substr(vstart);
        }
      }
    } else if (line.find("AllowedIPs") == 0) {
      size_t eq = line.find('=');
      if (eq != string::npos) {
        string value = line.substr(eq + 1);
        size_t vstart = value.find_first_not_of(" ");
        if (vstart != string::npos) {
          // Remove /32 suffix
          string ip = value.substr(vstart);
          size_t slash = ip.find('/');
          if (slash != string::npos) {
            ip = ip.substr(0, slash);
          }
          current_peer["vpn_ip"] = ip;
        }
      }
    }
  }

  // Don't forget the last peer
  if (!current_peer.empty()) {
    peers.push_back(current_peer);
  }

  return CmdResult(0, peers.dump(2) + "\n");
}

CmdResult handleGetWireGuardIp(const json &) {
  // Get local WireGuard IP from wg0 interface
  string cmd = "ip -4 addr show wg0 2>/dev/null | grep -oP '(?<=inet\\s)\\d+\\.\\d+\\.\\d+\\.\\d+'";

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return CmdResult(1, "Failed to query wg0 interface\n");
  }

  char buffer[64];
  string ip;
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    ip = buffer;
    // Trim newline
    if (!ip.empty() && ip.back() == '\n') {
      ip.pop_back();
    }
  }

  int exit_code = pclose(pipe);
  int actual_exit_code = WEXITSTATUS(exit_code);

  if (actual_exit_code != 0 || ip.empty()) {
    return CmdResult(1, "WireGuard interface wg0 not found or has no IP\n");
  }

  return CmdResult(0, ip + "\n");
}
