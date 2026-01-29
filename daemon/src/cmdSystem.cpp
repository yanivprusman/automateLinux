#include "cmdSystem.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include "main.h"
#include <sstream>

using namespace std;

// External globals
extern const CommandSignature COMMAND_REGISTRY[];
extern const size_t COMMAND_REGISTRY_SIZE;

static const string HELP_MESSAGE =
    "Usage: d <command> [options]\n"
    "       d <command> --help\n\n"
    "automateLinux daemon - Central service for Linux desktop automation.\n\n"
    "COMMON COMMANDS\n"
    "  ping                    Check daemon is running (returns 'pong')\n"
    "  help, --help            Show this help message\n"
    "  listCommands            List all available commands\n\n"
    "KEYBOARD/INPUT\n"
    "  enableKeyboard          Enable keyboard input grabbing\n"
    "  disableKeyboard         Disable keyboard input grabbing\n"
    "  getKeyboardEnabled      Check if keyboard is enabled\n"
    "  simulateInput           Simulate input events or type text\n"
    "                          --string \"text\" OR --type --code --value\n\n"
    "PORT MANAGEMENT\n"
    "  listPorts               List all port assignments\n"
    "  getPort --key <app>     Get assigned port for an app\n"
    "  setPort --key <app> --value <port>\n"
    "                          Assign port to an app\n"
    "  deletePort --key <app>  Remove port assignment\n\n"
    "PEER NETWORKING\n"
    "  setPeerConfig           Configure peer role and identity\n"
    "                          --role (leader|worker) --id <name>\n"
    "                          [--leader <ip>] (for workers)\n"
    "  getPeerStatus           Show peer config and connection status\n"
    "  listPeers               List all peers in the network\n"
    "  getPeerInfo --peer <id> Get detailed info about a peer\n\n"
    "APP ASSIGNMENTS (prevents git conflicts)\n"
    "  claimApp --app <name>   Claim exclusive work on an extraApp\n"
    "  releaseApp --app <name> Release app assignment\n"
    "                          [--force true] to override ownership\n"
    "  listApps                List all app assignments\n"
    "  getAppOwner --app <name>\n"
    "                          Check which peer owns an app\n\n"
    "LOOM (screen streaming)\n"
    "  restartLoom             Start/restart Loom streaming\n"
    "  stopLoom                Stop Loom streaming\n"
    "  isLoomActive            Check if Loom is active\n\n"
    "LOGGING\n"
    "  shouldLog --enable <bool>\n"
    "                          Enable or disable logging\n"
    "  registerLogListener     Register for live log streaming\n"
    "  addLogFilter            Add input event log filter\n"
    "  listLogFilters          List active log filters\n"
    "  clearLogFilters         Clear all log filters\n\n"
    "DATABASE\n"
    "  showDB                  Show database summary\n"
    "  getEntry --key <k>      Get a setting value\n"
    "  upsertEntry --key <k> --value <v>\n"
    "                          Set a setting value\n"
    "  deleteEntry --key <k>   Delete a setting\n\n"
    "OPTIONS\n"
    "  --help, -h              Show help for a specific command\n"
    "  --json                  Output in JSON format (some commands)\n\n"
    "EXAMPLES\n"
    "  d ping\n"
    "  d listPeers\n"
    "  d claimApp --app cad\n"
    "  d setPeerConfig --role leader --id desktop\n"
    "  d simulateInput --string \"Hello World\"\n"
    "  d getPort --key dashboard-dev\n\n"
    "Run 'd <command> --help' for detailed help on any command.\n"
    "See also: man daemon(1)\n";

CmdResult handleHelp(const json &) { return CmdResult(0, HELP_MESSAGE); }

CmdResult handlePing(const json &) { return CmdResult(0, "pong\n"); }

CmdResult handleQuit(const json &) {
  running = 0; // Signal the daemon to shut down
  return CmdResult(0, "Shutting down daemon.\n");
}

CmdResult handleGetDir(const json &command) {
  string dirName = command[COMMAND_ARG_DIR_NAME].get<string>();
  string result;
  if (dirName == "base") {
    result = directories.base;
  } else if (dirName == "data") {
    result = directories.data;
  } else {
    return CmdResult(1, "Unknown directory name: " + dirName + "\n");
  }
  return CmdResult(0, result + "\n");
}

CmdResult handleGetFile(const json &command) {
  string fileName = command[COMMAND_ARG_FILE_NAME].get<string>();
  for (const auto &file : files.files) {
    if (file.name.find(fileName) != string::npos) {
      return CmdResult(0, file.fullPath() + "\n");
    }
  }
  return CmdResult(1, "File not found: " + fileName + "\n");
}

CmdResult handleGetSocketPath(const json &) {
  return CmdResult(0, SOCKET_PATH + string("\n"));
}

CmdResult handleListCommands(const json &) {
  std::stringstream ss;
  for (size_t i = 0; i < COMMAND_REGISTRY_SIZE; ++i) {
    if (!COMMAND_REGISTRY[i].name.empty()) {
      ss << COMMAND_REGISTRY[i].name << "\n";
    }
  }
  return CmdResult(0, ss.str());
}

CmdResult handleTestLsof(const json &command) {
  int port = command[COMMAND_ARG_PORT].get<int>();
  string lsofCmd = "/usr/bin/lsof -i :" + to_string(port) + " 2>&1";
  string output = executeCommand(lsofCmd.c_str());
  return CmdResult(0, output + "\n");
}

CmdResult handleTestEcho(const json &command) {
  string message = command[COMMAND_ARG_MESSAGE].get<string>();
  string echoCmd = "echo " + message;
  string output = executeCommand(echoCmd.c_str());
  return CmdResult(0, output + "\n");
}

CmdResult handleTestLsofScript(const json &command) {
  string port = command[COMMAND_ARG_PORT].get<string>();
  string scriptPath =
      directories.base + "daemon/scripts/test_lsof.sh " + port;
  string output = executeCommand(scriptPath.c_str());
  return CmdResult(0, output + "\n");
}
