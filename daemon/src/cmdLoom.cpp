#include "cmdLoom.h"
#include "cmdApp.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "Utils.h"
#include <sstream>

using namespace std;

CmdResult handleResetClock(const json &) {
  SettingsTable::deleteSetting("clockX");
  SettingsTable::deleteSetting("clockY");
  return CmdResult(0, "Clock position reset (entries deleted)\n");
}

CmdResult handleIsLoomActive(const json &) {
  // Use AppManager functions for port checking
  bool serverProdRunning = AppManager::isPortListening(3500);
  bool serverDevRunning = AppManager::isPortListening(3505);
  bool clientProdRunning = AppManager::isPortListening(3004);
  bool clientDevRunning = AppManager::isPortListening(3005);

  std::stringstream ss;
  ss << "Loom Status:\n";
  ss << "  Server (Prod:3500): "
     << (serverProdRunning ? "RUNNING" : "NOT RUNNING") << "\n";
  ss << "  Server (Dev:3505): "
     << (serverDevRunning ? "RUNNING" : "NOT RUNNING") << "\n";
  ss << "  Client (Prod:3004): "
     << (clientProdRunning ? "RUNNING" : "NOT RUNNING") << "\n";
  ss << "  Client (Dev:3005): "
     << (clientDevRunning ? "RUNNING" : "NOT RUNNING") << "\n";

  return CmdResult(0, ss.str());
}

CmdResult handleStopLoom(const json &command) {
  // Redirect to generic app handler
  json appCmd;
  appCmd["app"] = "loom";
  appCmd["mode"] = command.contains("mode") ? command["mode"].get<string>() : "all";
  return handleStopApp(appCmd);
}

CmdResult handleRestartLoom(const json &command) {
  // Redirect to generic app handler
  json appCmd;
  appCmd["app"] = "loom";
  appCmd["mode"] = command.contains("mode") ? command["mode"].get<string>() : "prod";
  return handleRestartApp(appCmd);
}

CmdResult handleGenerateLoomToken(const json &) {
  // Use port from registry if possible
  string portStr = SettingsTable::getSetting("port_loom-server");
  if (portStr.empty())
    portStr = "3500";

  string cmd =
      "curl -s --max-time 2 http://localhost:" + portStr + "/api/generateToken";
  string output = executeCommand(cmd.c_str());

  if (output.empty()) {
    return CmdResult(
        1, "Error: Loom server not responding or not running on port " +
               portStr + "\n");
  }

  try {
    auto j = json::parse(output);
    if (j.contains("token")) {
      string token = j["token"].get<string>();
      // Construct link - assume client is on 3004
      return CmdResult(0, "Token: " + token +
                              "\nLink: http://localhost:3004/?token=" + token +
                              "\n");
    }
  } catch (...) {
  }

  return CmdResult(1, "Error: Unexpected response from Loom server: " + output +
                          "\n");
}

CmdResult handleRevokeLoomTokens(const json &) {
  string portStr = SettingsTable::getSetting("port_loom-server");
  if (portStr.empty())
    portStr = "3500";

  string cmd =
      "curl -s --max-time 2 http://localhost:" + portStr + "/api/revokeAll";
  executeCommand(cmd.c_str());

  return CmdResult(0, "Loom tokens revoked.\n");
}

CmdResult handleLoomConnect(const json &command) {
  string peer = "desktop"; // default
  if (command.contains(COMMAND_ARG_PEER)) {
    peer = command[COMMAND_ARG_PEER].get<string>();
  }

  // Validate peer
  if (peer != "desktop" && peer != "vps" && peer != "laptop") {
    return CmdResult(1, "Error: Unknown peer '" + peer +
                            "'. Valid peers: desktop, vps, laptop\n");
  }

  // Launch GUI app as user with Wayland display environment
  // Use nohup and setsid to fully detach from daemon
  string cmd = "setsid sudo -u yaniv "
               "WAYLAND_DISPLAY=wayland-0 "
               "XDG_RUNTIME_DIR=/run/user/1000 "
               "nohup /opt/automateLinux/extraApps/loom/native-client/build/loom-client "
               "--peer " + peer + " >/tmp/loom-client.log 2>&1 &";

  system(cmd.c_str());

  return CmdResult(0, "Launching loom client connecting to " + peer + "...\n");
}
