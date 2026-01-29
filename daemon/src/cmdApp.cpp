#include "cmdApp.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "PeerManager.h"
#include "Utils.h"
#include <cstring>
#include <fstream>
#include <unistd.h>

using namespace std;
using ordered_json = nlohmann::ordered_json;

// Forward declaration - implemented in DaemonServer.cpp
extern string getWgInterfaceIP();

CmdResult handleClaimApp(const json &command) {
  string app_name = command[COMMAND_ARG_APP].get<string>();
  PeerManager &pm = PeerManager::getInstance();
  string my_peer_id = pm.getPeerId();

  if (my_peer_id.empty()) {
    my_peer_id = "local";  // Default if peer not configured
  }

  // If we're a worker connected to leader, forward the request
  if (!pm.isLeader() && pm.isConnectedToLeader()) {
    json fwdCmd;
    fwdCmd["command"] = COMMAND_CLAIM_APP;
    fwdCmd[COMMAND_ARG_APP] = app_name;
    fwdCmd["requesting_peer"] = my_peer_id;

    int leaderFd = pm.getLeaderSocket();
    string msg = fwdCmd.dump() + "\n";
    if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
      return CmdResult(1, "Failed to forward claimApp to leader\n");
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

  // Leader logic: check requesting_peer if this is forwarded from a worker
  string requesting_peer = my_peer_id;
  if (command.contains("requesting_peer")) {
    requesting_peer = command["requesting_peer"].get<string>();
  }

  // Check if app is already assigned
  string current_owner = AppAssignmentTable::getOwner(app_name);
  if (!current_owner.empty() && current_owner != requesting_peer) {
    AppAssignment assignment = AppAssignmentTable::getAssignment(app_name);
    return CmdResult(0, "WARNING: " + app_name + " is assigned to " +
                            current_owner + " since " + assignment.assigned_at +
                            "\n");
  }

  // Assign to the requesting peer
  AppAssignmentTable::assignApp(app_name, requesting_peer);

  // Get the dev port for this app
  string port_key = "port_" + app_name + "-dev";
  string port_str = SettingsTable::getSetting(port_key);

  string result_msg = "Claimed " + app_name;
  if (!port_str.empty()) {
    result_msg += " (dev port " + port_str + ")";

    // If leader, notify VPS to update nginx forwarding
    if (pm.isLeader() && requesting_peer != "vps") {
      string peer_ip = PeerTable::getIpAddress(requesting_peer);
      // If requesting_peer is the leader itself, use own wg0 IP
      if (peer_ip.empty() && requesting_peer == pm.getPeerId()) {
        peer_ip = getWgInterfaceIP();
      }
      if (!peer_ip.empty()) {
        json nginxCmd;
        nginxCmd["command"] = COMMAND_UPDATE_NGINX_FORWARD;
        nginxCmd[COMMAND_ARG_PORT] = port_str;
        nginxCmd[COMMAND_ARG_TARGET] = peer_ip;
        if (pm.sendToPeer("vps", nginxCmd)) {
          result_msg += ", VPS forwarding to " + peer_ip;
          logToFile("Sent nginx forward to VPS: port " + port_str + " -> " +
                        peer_ip,
                    LOG_CORE);
        } else {
          logToFile("Failed to send nginx forward to VPS (not connected?)",
                    LOG_CORE);
        }
      }
    }

    logToFile("App claimed: " + app_name + " by " + requesting_peer +
                  ", dev port " + port_str,
              LOG_CORE);
  }

  return CmdResult(0, result_msg + "\n");
}

CmdResult handleReleaseApp(const json &command) {
  string app_name = command[COMMAND_ARG_APP].get<string>();
  PeerManager &pm = PeerManager::getInstance();
  string my_peer_id = pm.getPeerId();

  if (my_peer_id.empty()) {
    my_peer_id = "local";
  }

  // If we're a worker connected to leader, forward the request
  if (!pm.isLeader() && pm.isConnectedToLeader()) {
    json fwdCmd;
    fwdCmd["command"] = COMMAND_RELEASE_APP;
    fwdCmd[COMMAND_ARG_APP] = app_name;
    fwdCmd["requesting_peer"] = my_peer_id;

    int leaderFd = pm.getLeaderSocket();
    string msg = fwdCmd.dump() + "\n";
    if (write(leaderFd, msg.c_str(), msg.length()) < 0) {
      return CmdResult(1, "Failed to forward releaseApp to leader\n");
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

  // Leader logic: check requesting_peer if this is forwarded from a worker
  string requesting_peer = my_peer_id;
  if (command.contains("requesting_peer")) {
    requesting_peer = command["requesting_peer"].get<string>();
  }

  // Check if this peer owns the app
  string current_owner = AppAssignmentTable::getOwner(app_name);
  if (current_owner.empty()) {
    return CmdResult(0, app_name + " is not assigned to anyone.\n");
  }

  bool force = false;
  if (command.contains(COMMAND_ARG_FORCE)) {
    auto &val = command[COMMAND_ARG_FORCE];
    force = val.is_boolean() ? val.get<bool>() : (val.get<string>() == "true");
  }

  if (current_owner != requesting_peer && !force) {
    return CmdResult(1, "Cannot release " + app_name + " - owned by " +
                            current_owner + " (use --force to override)\n");
  }

  AppAssignmentTable::releaseApp(app_name);
  logToFile("App released: " + app_name + " by " + requesting_peer +
                (force ? " (forced)" : ""),
            LOG_CORE);

  return CmdResult(0, "Released " + app_name + "\n");
}

CmdResult handleListApps(const json &) {
  auto assignments = AppAssignmentTable::getAllAssignments();
  ordered_json result = ordered_json::array();

  for (const auto &assignment : assignments) {
    ordered_json a;
    a["app_name"] = assignment.app_name;
    a["assigned_peer"] = assignment.assigned_peer;
    a["assigned_at"] = assignment.assigned_at;
    a["last_activity"] = assignment.last_activity;
    result.push_back(a);
  }

  return CmdResult(0, result.dump(2) + "\n");
}

CmdResult handleGetAppOwner(const json &command) {
  string app_name = command[COMMAND_ARG_APP].get<string>();
  string owner = AppAssignmentTable::getOwner(app_name);

  if (owner.empty()) {
    return CmdResult(0, app_name + " is not assigned.\n");
  }

  AppAssignment assignment = AppAssignmentTable::getAssignment(app_name);
  return CmdResult(0, app_name + " is assigned to " + owner + " since " +
                          assignment.assigned_at + "\n");
}

CmdResult handleUpdateNginxForward(const json &command) {
  int port = 0;
  if (command[COMMAND_ARG_PORT].is_number()) {
    port = command[COMMAND_ARG_PORT].get<int>();
  } else {
    port = std::stoi(command[COMMAND_ARG_PORT].get<string>());
  }
  string target_ip = command[COMMAND_ARG_TARGET].get<string>();

  // Generate nginx config for this port
  string config_path = "/etc/nginx/conf.d/daemon-forward-" + to_string(port) +
                       ".conf";

  string nginx_config = "# Auto-generated by daemon. DO NOT EDIT.\n"
                        "server {\n"
                        "    listen " +
                        to_string(port) +
                        ";\n"
                        "    location / {\n"
                        "        proxy_pass http://" +
                        target_ip + ":" + to_string(port) +
                        ";\n"
                        "        proxy_http_version 1.1;\n"
                        "        proxy_set_header Upgrade $http_upgrade;\n"
                        "        proxy_set_header Connection \"upgrade\";\n"
                        "        proxy_set_header Host $host;\n"
                        "    }\n"
                        "}\n";

  // Write config file
  std::ofstream ofs(config_path);
  if (!ofs.is_open()) {
    return CmdResult(1, "Failed to write nginx config to " + config_path +
                            ". Check permissions.\n");
  }
  ofs << nginx_config;
  ofs.close();

  // Reload nginx
  int rc = system("nginx -t && systemctl reload nginx");
  if (rc != 0) {
    return CmdResult(1, "Nginx config written but reload failed. Check nginx "
                        "config syntax.\n");
  }

  logToFile("Nginx forward updated: port " + to_string(port) + " -> " +
                target_ip,
            LOG_CORE);
  return CmdResult(0, "Port " + to_string(port) + " now forwards to " +
                          target_ip + "\n");
}
