#include "cmdApp.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

using namespace std;

// Static app configurations - hardcoded for now, can move to DB later
static const vector<AppConfig> APP_CONFIGS = {
    {"loom", "Loom Screen Streaming", true, "loom-server-{mode}",
     "loom-client-{mode}", "loom", "loom-server",
     "/opt/automateLinux/extraApps/loom", "/opt/prod/loom", "server", "client"},
    {"cad", "CAD Application", false, "", "cad-{mode}", "cad", "",
     "/opt/automateLinux/extraApps/cad", "/opt/prod/cad", "", "web"},
    {"pt", "Public Transportation", false, "", "pt-{mode}", "pt", "",
     "/opt/automateLinux/extraApps/publicTransportation",
     "/opt/prod/publicTransportation", "", "web"},
    {"dashboard", "Dashboard", true, "dashboard-bridge", "dashboard-{mode}",
     "dashboard", "dashboard-bridge", "/opt/automateLinux/dashboard",
     "/opt/automateLinux/dashboard", "", ""}};

// ============================================================================
// AppManager namespace implementation
// ============================================================================

bool AppManager::startService(const string &serviceName) {
  string cmd = "/usr/bin/systemctl start " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

bool AppManager::stopService(const string &serviceName) {
  string cmd = "/usr/bin/systemctl stop " + serviceName + " 2>/dev/null";
  std::system(cmd.c_str());
  // Also reset-failed to allow immediate restart
  cmd = "/usr/bin/systemctl reset-failed " + serviceName + " 2>/dev/null";
  std::system(cmd.c_str());
  return true;
}

bool AppManager::restartService(const string &serviceName) {
  string cmd = "/usr/bin/systemctl restart " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

bool AppManager::enableService(const string &serviceName) {
  string cmd = "/usr/bin/systemctl enable " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

bool AppManager::disableService(const string &serviceName) {
  string cmd = "/usr/bin/systemctl disable " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

bool AppManager::isServiceActive(const string &serviceName) {
  string cmd =
      "/usr/bin/systemctl is-active --quiet " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

bool AppManager::isServiceEnabled(const string &serviceName) {
  string cmd =
      "/usr/bin/systemctl is-enabled --quiet " + serviceName + " 2>/dev/null";
  int rc = std::system(cmd.c_str());
  return (rc == 0);
}

string AppManager::getServiceStatus(const string &serviceName) {
  string cmd = "/usr/bin/systemctl status " + serviceName +
               " --no-pager -l 2>/dev/null | head -10";
  return executeCommand(cmd.c_str());
}

bool AppManager::isPortListening(int port) {
  string cmd = "/usr/bin/ss -tlnH sport = :" + to_string(port);
  return !executeCommand(cmd.c_str()).empty();
}

void AppManager::killProcessOnPort(int port) {
  string cmd = "/usr/bin/fuser -k -9 " + to_string(port) + "/tcp 2>/dev/null";
  std::system(cmd.c_str());
}

bool AppManager::waitForPortRelease(int port, int timeoutMs) {
  int elapsedMs = 0;
  const int pollIntervalMs = 200;

  while (isPortListening(port) && elapsedMs < timeoutMs) {
    // Force kill after 2.5 seconds
    if (elapsedMs > 2500) {
      killProcessOnPort(port);
    }
    this_thread::sleep_for(chrono::milliseconds(pollIntervalMs));
    elapsedMs += pollIntervalMs;
  }
  return !isPortListening(port);
}

AppConfig AppManager::getAppConfig(const string &appId) {
  for (const auto &cfg : APP_CONFIGS) {
    if (cfg.appId == appId) {
      return cfg;
    }
  }
  return AppConfig{}; // Empty config if not found
}

vector<AppConfig> AppManager::getAllApps() { return APP_CONFIGS; }

string AppManager::resolveServiceName(const string &templateStr,
                                      const string &appId,
                                      const string &mode) {
  string result = templateStr;
  // Replace {app} with appId
  size_t pos = result.find("{app}");
  if (pos != string::npos) {
    result.replace(pos, 5, appId);
  }
  // Replace {mode} with mode
  pos = result.find("{mode}");
  if (pos != string::npos) {
    result.replace(pos, 6, mode);
  }
  return result;
}

string AppManager::getAppPath(const string &appId, const string &mode) {
  AppConfig cfg = getAppConfig(appId);
  if (cfg.appId.empty())
    return "";
  return (mode == "prod") ? cfg.prodPath : cfg.devPath;
}

string AppManager::buildServerComponent(const string &appId,
                                        const string &mode) {
  AppConfig cfg = getAppConfig(appId);
  if (cfg.appId.empty())
    return "Error: Unknown app: " + appId + "\n";

  if (!cfg.hasServerComponent || cfg.serverBuildSubdir.empty())
    return "Error: App " + appId + " has no server component to build\n";

  string appPath = getAppPath(appId, mode);
  string serverPath = appPath + "/" + cfg.serverBuildSubdir;
  string buildDir = serverPath + "/build";

  stringstream result;
  result << "Building " << appId << " server (" << mode << ")...\n";
  result << "  Path: " << serverPath << "\n";

  // Create build directory
  string mkdirCmd = "/usr/bin/mkdir -p " + buildDir;
  if (std::system(mkdirCmd.c_str()) != 0) {
    return result.str() + "Error: Failed to create build directory\n";
  }

  // Run cmake
  string cmakeCmd =
      "cd " + buildDir + " && /usr/bin/cmake .. 2>&1";
  string cmakeOutput = executeCommand(cmakeCmd.c_str());
  if (cmakeOutput.find("Error") != string::npos ||
      cmakeOutput.find("error") != string::npos) {
    return result.str() + "CMake output:\n" + cmakeOutput + "\n";
  }
  result << "  CMake: OK\n";

  // Run make
  string makeCmd = "cd " + buildDir + " && /usr/bin/make -j$(nproc) 2>&1";
  string makeOutput = executeCommand(makeCmd.c_str());
  if (makeOutput.find("Error") != string::npos ||
      makeOutput.find("error:") != string::npos) {
    return result.str() + "Make output:\n" + makeOutput + "\n";
  }
  result << "  Make: OK\n";
  result << "Build complete.\n";

  return result.str();
}

string AppManager::installDependencies(const string &appId, const string &mode,
                                       const string &component) {
  AppConfig cfg = getAppConfig(appId);
  if (cfg.appId.empty())
    return "Error: Unknown app: " + appId + "\n";

  string appPath = getAppPath(appId, mode);
  string targetDir;

  if (component == "client") {
    if (cfg.clientSubdir.empty())
      return "Error: App " + appId + " has no client component\n";
    targetDir = appPath + "/" + cfg.clientSubdir;
  } else if (component == "server") {
    if (cfg.serverBuildSubdir.empty())
      return "Error: App " + appId + " has no server component\n";
    targetDir = appPath + "/" + cfg.serverBuildSubdir;
  } else if (component == "all" || component.empty()) {
    // Install for all components
    stringstream result;
    if (!cfg.clientSubdir.empty()) {
      result << installDependencies(appId, mode, "client");
    }
    if (!cfg.serverBuildSubdir.empty()) {
      result << installDependencies(appId, mode, "server");
    }
    return result.str();
  } else {
    return "Error: Unknown component: " + component +
           " (use 'client', 'server', or 'all')\n";
  }

  stringstream result;
  result << "Installing dependencies for " << appId << " " << component << " ("
         << mode << ")...\n";
  result << "  Path: " << targetDir << "\n";

  // Check if package.json exists
  string checkCmd = "/usr/bin/test -f " + targetDir + "/package.json";
  if (std::system(checkCmd.c_str()) != 0) {
    return result.str() + "  No package.json found, skipping.\n";
  }

  // Run npm install
  string npmCmd = "cd " + targetDir + " && /usr/bin/npm install 2>&1";
  string npmOutput = executeCommand(npmCmd.c_str());

  // Check for errors
  if (npmOutput.find("ERR!") != string::npos ||
      npmOutput.find("error") != string::npos) {
    return result.str() + "npm output:\n" + npmOutput + "\n";
  }

  result << "  npm install: OK\n";
  return result.str();
}

// ============================================================================
// Command handlers
// ============================================================================

CmdResult handleStartApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "prod";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  stringstream result;

  // Start server if app has one
  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService =
        AppManager::resolveServiceName(config.serverServiceTemplate, appId, mode);
    if (AppManager::startService(serverService)) {
      result << "Started " << serverService << "\n";
    } else {
      result << "Failed to start " << serverService << "\n";
    }
  }

  // Start client
  if (!config.clientServiceTemplate.empty()) {
    string clientService =
        AppManager::resolveServiceName(config.clientServiceTemplate, appId, mode);
    if (AppManager::startService(clientService)) {
      result << "Started " << clientService << "\n";
    } else {
      result << "Failed to start " << clientService << "\n";
    }
  }

  return CmdResult(0, result.str());
}

CmdResult handleStopApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "all";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  vector<string> modes;
  if (mode == "all") {
    modes = {"prod", "dev"};
  } else {
    modes = {mode};
  }

  stringstream result;

  for (const auto &m : modes) {
    // Stop client first
    if (!config.clientServiceTemplate.empty()) {
      string clientService =
          AppManager::resolveServiceName(config.clientServiceTemplate, appId, m);
      AppManager::stopService(clientService);
      result << "Stopped " << clientService << "\n";
    }

    // Stop server
    if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
      string serverService =
          AppManager::resolveServiceName(config.serverServiceTemplate, appId, m);
      AppManager::stopService(serverService);
      result << "Stopped " << serverService << "\n";
    }

    // Kill leftover processes on ports
    string portKeyClient = "port_" + config.portKeyClient + "-" + m;
    string portStr = SettingsTable::getSetting(portKeyClient);
    if (!portStr.empty()) {
      int port = stoi(portStr);
      AppManager::killProcessOnPort(port);
    }

    if (config.hasServerComponent && !config.portKeyServer.empty()) {
      string portKeyServer = "port_" + config.portKeyServer;
      if (m == "dev")
        portKeyServer += "-dev";
      string serverPortStr = SettingsTable::getSetting(portKeyServer);
      if (!serverPortStr.empty()) {
        int serverPort = stoi(serverPortStr);
        AppManager::killProcessOnPort(serverPort);
      }
    }
  }

  return CmdResult(0, result.str());
}

CmdResult handleRestartApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "prod";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  stringstream result;

  // 1. Stop existing services
  if (!config.clientServiceTemplate.empty()) {
    string clientService =
        AppManager::resolveServiceName(config.clientServiceTemplate, appId, mode);
    AppManager::stopService(clientService);
  }

  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService =
        AppManager::resolveServiceName(config.serverServiceTemplate, appId, mode);
    AppManager::stopService(serverService);
  }

  // 2. Wait for ports to release
  string portKeyClient = "port_" + config.portKeyClient + "-" + mode;
  string portStr = SettingsTable::getSetting(portKeyClient);
  if (!portStr.empty()) {
    int port = stoi(portStr);
    AppManager::waitForPortRelease(port);
  }

  if (config.hasServerComponent && !config.portKeyServer.empty()) {
    string portKeyServer = "port_" + config.portKeyServer;
    if (mode == "dev")
      portKeyServer += "-dev";
    string serverPortStr = SettingsTable::getSetting(portKeyServer);
    if (!serverPortStr.empty()) {
      int serverPort = stoi(serverPortStr);
      AppManager::waitForPortRelease(serverPort);
    }
  }

  // 3. Start server first (if exists)
  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService =
        AppManager::resolveServiceName(config.serverServiceTemplate, appId, mode);
    if (AppManager::restartService(serverService)) {
      result << "Restarted " << serverService << "\n";
    } else {
      result << "Failed to restart " << serverService << "\n";
    }
  }

  // 4. Start client
  if (!config.clientServiceTemplate.empty()) {
    string clientService =
        AppManager::resolveServiceName(config.clientServiceTemplate, appId, mode);
    if (AppManager::restartService(clientService)) {
      result << "Restarted " << clientService << "\n";
    } else {
      result << "Failed to restart " << clientService << "\n";
    }
  }

  return CmdResult(0, result.str());
}

CmdResult handleAppStatus(const json &command) {
  string appId = command.contains("app") ? command["app"].get<string>() : "";

  vector<AppConfig> apps;
  if (appId.empty()) {
    apps = AppManager::getAllApps();
  } else {
    AppConfig cfg = AppManager::getAppConfig(appId);
    if (!cfg.appId.empty()) {
      apps.push_back(cfg);
    } else {
      return CmdResult(1, "Unknown app: " + appId + "\n");
    }
  }

  stringstream ss;

  for (const auto &app : apps) {
    ss << "=== " << app.displayName << " (" << app.appId << ") ===\n";

    for (const auto &mode : {"prod", "dev"}) {
      ss << "  [" << mode << "]\n";

      // Server status
      if (app.hasServerComponent && !app.serverServiceTemplate.empty()) {
        string svc =
            AppManager::resolveServiceName(app.serverServiceTemplate, app.appId, mode);
        bool active = AppManager::isServiceActive(svc);
        ss << "    Server (" << svc << "): " << (active ? "RUNNING" : "STOPPED")
           << "\n";
      }

      // Client status
      if (!app.clientServiceTemplate.empty()) {
        string clientSvc =
            AppManager::resolveServiceName(app.clientServiceTemplate, app.appId, mode);
        bool clientActive = AppManager::isServiceActive(clientSvc);
        ss << "    Client (" << clientSvc
           << "): " << (clientActive ? "RUNNING" : "STOPPED") << "\n";
      }

      // Port status
      string portKey = "port_" + app.portKeyClient + "-" + string(mode);
      string portStr = SettingsTable::getSetting(portKey);
      if (!portStr.empty()) {
        int port = stoi(portStr);
        bool listening = AppManager::isPortListening(port);
        ss << "    Port " << port << ": "
           << (listening ? "LISTENING" : "NOT LISTENING") << "\n";
      }
    }
  }

  return CmdResult(0, ss.str());
}

CmdResult handleListApps(const json &) {
  vector<AppConfig> apps = AppManager::getAllApps();

  stringstream ss;
  ss << "Registered Apps:\n";
  for (const auto &app : apps) {
    ss << "  - " << app.appId << ": " << app.displayName;
    if (app.hasServerComponent) {
      ss << " (has server)";
    }
    ss << "\n";
  }

  return CmdResult(0, ss.str());
}

CmdResult handleBuildApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "prod";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  if (!config.hasServerComponent) {
    return CmdResult(1, "App " + appId + " has no server component to build\n");
  }

  string result = AppManager::buildServerComponent(appId, mode);

  // Check if build succeeded
  bool success = result.find("Build complete") != string::npos;

  return CmdResult(success ? 0 : 1, result);
}

CmdResult handleInstallAppDeps(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "prod";
  string component =
      command.contains("component") ? command["component"].get<string>() : "all";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  string result = AppManager::installDependencies(appId, mode, component);

  // Check if install succeeded
  bool success = result.find("Error") == string::npos &&
                 result.find("ERR!") == string::npos;

  return CmdResult(success ? 0 : 1, result);
}

CmdResult handleEnableApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "prod";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  stringstream result;
  bool allSuccess = true;

  // Enable server if app has one
  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService =
        AppManager::resolveServiceName(config.serverServiceTemplate, appId, mode);
    if (AppManager::enableService(serverService)) {
      result << "Enabled " << serverService << "\n";
    } else {
      result << "Failed to enable " << serverService << "\n";
      allSuccess = false;
    }
  }

  // Enable client
  if (!config.clientServiceTemplate.empty()) {
    string clientService =
        AppManager::resolveServiceName(config.clientServiceTemplate, appId, mode);
    if (AppManager::enableService(clientService)) {
      result << "Enabled " << clientService << "\n";
    } else {
      result << "Failed to enable " << clientService << "\n";
      allSuccess = false;
    }
  }

  return CmdResult(allSuccess ? 0 : 1, result.str());
}

CmdResult handleDisableApp(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "all";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  vector<string> modes;
  if (mode == "all") {
    modes = {"prod", "dev"};
  } else {
    modes = {mode};
  }

  stringstream result;

  for (const auto &m : modes) {
    // Disable client
    if (!config.clientServiceTemplate.empty()) {
      string clientService =
          AppManager::resolveServiceName(config.clientServiceTemplate, appId, m);
      AppManager::disableService(clientService);
      result << "Disabled " << clientService << "\n";
    }

    // Disable server
    if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
      string serverService =
          AppManager::resolveServiceName(config.serverServiceTemplate, appId, m);
      AppManager::disableService(serverService);
      result << "Disabled " << serverService << "\n";
    }
  }

  return CmdResult(0, result.str());
}
