#include "cmdLoom.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "Utils.h"
#include <sstream>

using namespace std;

// Helper to check if a port is listening using ss (works without sudo, unlike lsof)
static bool isPortListening(int port) {
  string cmd = "/usr/bin/ss -tlnH sport = :" + std::to_string(port);
  return !executeCommand(cmd.c_str()).empty();
}

CmdResult handleResetClock(const json &) {
  SettingsTable::deleteSetting("clockX");
  SettingsTable::deleteSetting("clockY");
  return CmdResult(0, "Clock position reset (entries deleted)\n");
}

CmdResult handleIsLoomActive(const json &) {
  bool serverProdRunning = isPortListening(3500);
  bool serverDevRunning = isPortListening(3505);
  bool clientProdRunning = isPortListening(3004);
  bool clientDevRunning = isPortListening(3005);

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
  // Check for --mode argument (prod, dev, or all)
  string mode = "all";
  if (command.contains("mode")) {
    mode = command["mode"].get<string>();
  }

  string cmd =
      directories.base + "daemon/scripts/stop_loom.sh --" + mode +
      " > /dev/null 2>&1";

  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    return CmdResult(1, "Failed to execute stop script\n");
  }

  return CmdResult(0, "Loom stopped (" + mode + ").\n");
}

CmdResult handleRestartLoom(const json &command) {
  // Check for --mode argument (prod or dev, default: prod)
  string mode = "prod";
  if (command.contains("mode")) {
    mode = command["mode"].get<string>();
  }

  // 1. First run the stop script synchronously to ensure clean state
  string stopCmd =
      directories.base + "daemon/scripts/stop_loom.sh --" + mode +
      " > /dev/null 2>&1";
  std::system(stopCmd.c_str());

  // 2. Now launch the restart script
  // We use `nohup` and `&` to ensure it continues running in background.
  string cmd =
      directories.base + "daemon/scripts/restart_loom.sh --" +
      mode + " &";

  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    return CmdResult(1, "Failed to launch restart script\n");
  }

  return CmdResult(0, "");
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
