#include "cmdLoom.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "Utils.h"
#include "cmdApp.h"
#include <sstream>

using namespace std;

CmdResult handleResetClock(const json &) {
  SettingsTable::deleteSetting("clockX");
  SettingsTable::deleteSetting("clockY");
  return CmdResult(0, "Clock position reset (entries deleted)\n");
}

CmdResult handleIsLoomActive(const json &) {
  // Use AppManager functions for port checking (dev only)
  bool serverRunning = AppManager::isPortListening(3505);

  std::stringstream ss;
  ss << "Loom Status:\n";
  ss << "  Server (3505): " << (serverRunning ? "RUNNING" : "NOT RUNNING")
     << "\n";

  return CmdResult(0, ss.str());
}

CmdResult handleStopLoom(const json &command) {
  // Redirect to generic app handler
  json appCmd;
  appCmd["app"] = "loom";
  appCmd["mode"] =
      command.contains("mode") ? command["mode"].get<string>() : "all";
  return handleStopApp(appCmd);
}

CmdResult handleRestartLoom(const json &command) {
  // Redirect to generic app handler
  json appCmd;
  appCmd["app"] = "loom";
  appCmd["mode"] =
      command.contains("mode") ? command["mode"].get<string>() : "dev";
  return handleRestartApp(appCmd);
}

CmdResult handleGenerateLoomToken(const json &) {
  // Use port from registry if possible
  string portStr = SettingsTable::getSetting("port_loom-server-dev");
  if (portStr.empty())
    portStr = "3505";

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
      // Construct link - client is on 3005
      return CmdResult(0, "Token: " + token + "\n");
    }
  } catch (...) {
  }

  return CmdResult(1, "Error: Unexpected response from Loom server: " + output +
                          "\n");
}

CmdResult handleRevokeLoomTokens(const json &) {
  string portStr = SettingsTable::getSetting("port_loom-server-dev");
  if (portStr.empty())
    portStr = "3505";

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

  // Dev only - port 3505
  string port = "3505";

  // Launch GUI app as user with Wayland display environment
  // Use nohup and setsid to fully detach from daemon
  string cmd =
      "setsid sudo -u yaniv "
      "WAYLAND_DISPLAY=wayland-0 "
      "XDG_RUNTIME_DIR=/run/user/1000 "
      "nohup /opt/automateLinux/extraApps/loom/native-client/build/loom-client "
      "--peer " +
      peer + " --port " + port + " >/tmp/loom-client.log 2>&1 &";

  system(cmd.c_str());

  return CmdResult(0, "Launching loom client connecting to " + peer +
                          " (port " + port + ")...\n");
}
