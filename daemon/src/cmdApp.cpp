#include "cmdApp.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>
#include <thread>
#include <tuple>

using namespace std;

// Static app configurations - hardcoded for now, can move to DB later
static const vector<AppConfig> APP_CONFIGS = {
    {"cad", "CAD Application", true, "", "cad-{mode}", "cad", "",
     "/opt/automateLinux/extraApps/cad", "/opt/prod/cad", ".", "web"},
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

// Helper: Convert ExtraAppRecord to AppConfig
static AppConfig recordToConfig(const ExtraAppRecord &record) {
  AppConfig cfg;
  cfg.appId = record.app_id;
  cfg.displayName = record.display_name;
  cfg.hasServerComponent = record.has_server_component;
  cfg.serverServiceTemplate = record.server_service_template;
  cfg.clientServiceTemplate = record.client_service_template;
  cfg.portKeyClient = record.port_key_client;
  cfg.portKeyServer = record.port_key_server;
  cfg.devPath = record.dev_path;
  cfg.prodPath = record.prod_path;
  cfg.serverBuildSubdir = record.server_build_subdir;
  cfg.clientSubdir = record.client_subdir;
  return cfg;
}

AppConfig AppManager::getAppConfig(const string &appId) {
  // First check hardcoded apps
  for (const auto &cfg : APP_CONFIGS) {
    if (cfg.appId == appId) {
      return cfg;
    }
  }
  // Then check database
  ExtraAppRecord record = ExtraAppTable::getApp(appId);
  if (!record.app_id.empty()) {
    return recordToConfig(record);
  }
  return AppConfig{}; // Empty config if not found
}

vector<AppConfig> AppManager::getAllApps() {
  vector<AppConfig> allApps = APP_CONFIGS;
  // Add apps from database
  vector<ExtraAppRecord> dbApps = ExtraAppTable::getAllApps();
  for (const auto &record : dbApps) {
    // Avoid duplicates (in case app is both hardcoded and in DB)
    bool exists = false;
    for (const auto &cfg : allApps) {
      if (cfg.appId == record.app_id) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      allApps.push_back(recordToConfig(record));
    }
  }
  return allApps;
}

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

string AppManager::buildCppComponent(const string &path) {
  string buildDir = path + "/build";

  stringstream result;
  result << "  Path: " << path << "\n";

  // Clean rebuild: delete build directory first for consistency
  string rmCmd = "/usr/bin/rm -rf " + buildDir;
  if (std::system(rmCmd.c_str()) != 0) {
    result << "  Warning: Failed to remove old build directory\n";
  }
  result << "  Clean: OK\n";

  // Create build directory
  string mkdirCmd = "/usr/bin/mkdir -p " + buildDir;
  if (std::system(mkdirCmd.c_str()) != 0) {
    return result.str() + "Error: Failed to create build directory\n";
  }

  // Run cmake
  string cmakeCmd = "cd " + buildDir + " && /usr/bin/cmake .. 2>&1";
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

string AppManager::buildServerComponent(const string &appId,
                                        const string &mode) {
  AppConfig cfg = getAppConfig(appId);
  if (cfg.appId.empty())
    return "Error: Unknown app: " + appId + "\n";

  if (!cfg.hasServerComponent || cfg.serverBuildSubdir.empty())
    return "Error: App " + appId + " has no server component to build\n";

  string appPath = getAppPath(appId, mode);
  string serverPath = appPath + "/" + cfg.serverBuildSubdir;

  stringstream result;
  result << "Building " << appId << " server (" << mode << ")...\n";
  result << buildCppComponent(serverPath);

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
  string mode = command.contains("mode") ? command["mode"].get<string>() : "dev";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  // Validate mode for dev-only apps
  if (mode == "prod" && config.prodPath.empty()) {
    return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
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
    // For dev-only apps (empty prodPath), only include dev mode
    if (config.prodPath.empty()) {
      modes = {"dev"};
    } else {
      modes = {"prod", "dev"};
    }
  } else {
    // Validate explicit mode for dev-only apps
    if (mode == "prod" && config.prodPath.empty()) {
      return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
    }
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
  string mode = command.contains("mode") ? command["mode"].get<string>() : "dev";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  // Validate mode for dev-only apps
  if (mode == "prod" && config.prodPath.empty()) {
    return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
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
      // Skip prod mode for apps without a prodPath (dev-only apps)
      if (string(mode) == "prod" && app.prodPath.empty()) {
        continue;
      }
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
  string mode = command.contains("mode") ? command["mode"].get<string>() : "dev";
  string component = command.contains(COMMAND_ARG_COMPONENT)
                         ? command[COMMAND_ARG_COMPONENT].get<string>()
                         : "server";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  // Validate mode for dev-only apps
  if (mode == "prod" && config.prodPath.empty()) {
    return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
  }

  string appPath = AppManager::getAppPath(appId, mode);
  stringstream result;

  if (component == "server") {
    if (!config.hasServerComponent) {
      return CmdResult(1, "App " + appId + " has no server component to build\n");
    }
    result << AppManager::buildServerComponent(appId, mode);
  } else if (component == "native-client") {
    // Build native client from native-client subdirectory
    string nativeClientPath = appPath + "/native-client";
    string checkDir = "/usr/bin/test -d " + nativeClientPath;
    if (std::system(checkDir.c_str()) != 0) {
      return CmdResult(1, "No native-client directory found at: " + nativeClientPath + "\n");
    }
    result << "Building " << appId << " native-client (" << mode << ")...\n";
    result << AppManager::buildCppComponent(nativeClientPath);
  } else {
    // Try to build from custom component subdirectory
    string componentPath = appPath + "/" + component;
    string checkDir = "/usr/bin/test -d " + componentPath;
    if (std::system(checkDir.c_str()) != 0) {
      return CmdResult(1, "No " + component + " directory found at: " + componentPath + "\n");
    }
    result << "Building " << appId << " " << component << " (" << mode << ")...\n";
    result << AppManager::buildCppComponent(componentPath);
  }

  // Check if build succeeded
  bool success = result.str().find("Build complete") != string::npos;

  return CmdResult(success ? 0 : 1, result.str());
}

CmdResult handleInstallAppDeps(const json &command) {
  if (!command.contains("app")) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command["app"].get<string>();
  string mode = command.contains("mode") ? command["mode"].get<string>() : "dev";
  string component =
      command.contains("component") ? command["component"].get<string>() : "all";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  // Validate mode for dev-only apps
  if (mode == "prod" && config.prodPath.empty()) {
    return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
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
  string mode = command.contains("mode") ? command["mode"].get<string>() : "dev";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  // Validate mode for dev-only apps
  if (mode == "prod" && config.prodPath.empty()) {
    return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
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
    // For dev-only apps (empty prodPath), only include dev mode
    if (config.prodPath.empty()) {
      modes = {"dev"};
    } else {
      modes = {"prod", "dev"};
    }
  } else {
    // Validate explicit mode for dev-only apps
    if (mode == "prod" && config.prodPath.empty()) {
      return CmdResult(1, appId + " is dev-only (no prod mode available)\n");
    }
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

// ============================================================================
// Extra App Management
// ============================================================================

// Helper: Extract app name from repo URL
// e.g., https://github.com/user/myapp.git -> myapp
static string extractAppNameFromUrl(const string &repoUrl) {
  // Find last '/' and extract everything after it
  size_t lastSlash = repoUrl.rfind('/');
  if (lastSlash == string::npos) {
    return "";
  }
  string name = repoUrl.substr(lastSlash + 1);
  // Remove .git suffix if present
  if (name.size() > 4 && name.substr(name.size() - 4) == ".git") {
    name = name.substr(0, name.size() - 4);
  }
  return name;
}

// Helper: Find next available port pair for apps (3000-3499 range)
static pair<int, int> findNextAvailablePortPair() {
  // Get all existing port assignments
  auto settings = SettingsTable::getAllSettings();
  set<int> usedPorts;
  for (const auto &[key, value] : settings) {
    if (key.find("port_") == 0) {
      try {
        usedPorts.insert(stoi(value));
      } catch (...) {}
    }
  }

  // Find first available pair in 3000-3499 range
  for (int base = 3000; base < 3500; base += 2) {
    if (usedPorts.find(base) == usedPorts.end() &&
        usedPorts.find(base + 1) == usedPorts.end()) {
      return {base, base + 1};
    }
  }
  return {-1, -1}; // No ports available
}

// Helper: Find next available server port (3500+ range)
static int findNextAvailableServerPort() {
  auto settings = SettingsTable::getAllSettings();
  set<int> usedPorts;
  for (const auto &[key, value] : settings) {
    if (key.find("port_") == 0) {
      try {
        usedPorts.insert(stoi(value));
      } catch (...) {}
    }
  }

  // Find first available port in 3500+ range
  for (int port = 3500; port < 4000; port++) {
    if (usedPorts.find(port) == usedPorts.end()) {
      return port;
    }
  }
  return -1;
}

// Helper: Detect app structure (hasServer, serverSubdir, clientSubdir)
static tuple<bool, string, string> detectAppStructure(const string &appPath) {
  bool hasServer = false;
  string serverSubdir = "";
  string clientSubdir = "";

  // Check for common server directories
  vector<string> serverDirs = {"server", "backend", "api"};
  for (const auto &dir : serverDirs) {
    string checkPath = appPath + "/" + dir;
    string checkCmd = "/usr/bin/test -d " + checkPath;
    if (std::system(checkCmd.c_str()) == 0) {
      // Check if it has CMakeLists.txt (C++ server)
      string cmakeCheck = "/usr/bin/test -f " + checkPath + "/CMakeLists.txt";
      if (std::system(cmakeCheck.c_str()) == 0) {
        hasServer = true;
        serverSubdir = dir;
        break;
      }
    }
  }

  // Check for common client directories
  vector<string> clientDirs = {"client", "web", "frontend", "app"};
  for (const auto &dir : clientDirs) {
    string checkPath = appPath + "/" + dir;
    string checkCmd = "/usr/bin/test -d " + checkPath;
    if (std::system(checkCmd.c_str()) == 0) {
      // Check if it has package.json (Node/React app)
      string pkgCheck = "/usr/bin/test -f " + checkPath + "/package.json";
      if (std::system(pkgCheck.c_str()) == 0) {
        clientSubdir = dir;
        break;
      }
    }
  }

  // If no client subdir found, check root for package.json
  if (clientSubdir.empty()) {
    string pkgCheck = "/usr/bin/test -f " + appPath + "/package.json";
    if (std::system(pkgCheck.c_str()) == 0) {
      clientSubdir = ""; // Root level
    }
  }

  return {hasServer, serverSubdir, clientSubdir};
}

CmdResult handleAddExtraApp(const json &command) {
  if (!command.contains(COMMAND_ARG_REPO_URL)) {
    return CmdResult(1, "Missing required argument: --repoUrl\n");
  }

  string repoUrl = command[COMMAND_ARG_REPO_URL].get<string>();
  string appId = extractAppNameFromUrl(repoUrl);

  if (appId.empty()) {
    return CmdResult(1, "Could not extract app name from URL: " + repoUrl + "\n");
  }

  // Check if app already exists in hardcoded list
  for (const auto &cfg : APP_CONFIGS) {
    if (cfg.appId == appId) {
      return CmdResult(1, "App already exists in hardcoded config: " + appId + "\n");
    }
  }

  // Check if app already exists in database
  if (ExtraAppTable::appExists(appId)) {
    return CmdResult(1, "App already registered in database: " + appId + "\n");
  }

  stringstream result;
  result << "Adding extra app: " << appId << "\n";

  string devPath = string(EXTRA_APPS_DIR) + appId;
  string prodPath = string(PROD_APPS_DIR) + appId;

  // 1. Check if directory exists, if not clone it
  string checkDir = "/usr/bin/test -d " + devPath;
  bool needsClone = (std::system(checkDir.c_str()) != 0);

  if (needsClone) {
    result << "  Cloning repository to " << devPath << "...\n";
    string cloneCmd = "/usr/bin/git clone " + repoUrl + " " + devPath + " 2>&1";
    string cloneOutput = executeCommand(cloneCmd.c_str());
    if (cloneOutput.find("fatal") != string::npos ||
        cloneOutput.find("error") != string::npos) {
      return CmdResult(1, "Failed to clone repository:\n" + cloneOutput + "\n");
    }
    result << "  Clone: OK\n";
  } else {
    result << "  Directory already exists: " << devPath << "\n";
  }

  // 2. Create prod worktree if it doesn't exist
  string checkProd = "/usr/bin/test -d " + prodPath;
  if (std::system(checkProd.c_str()) != 0) {
    result << "  Creating prod worktree at " << prodPath << "...\n";

    // First ensure /opt/prod exists
    std::system("/usr/bin/mkdir -p /opt/prod");

    // Get current commit hash
    string getHashCmd = "cd " + devPath + " && /usr/bin/git rev-parse HEAD";
    string commitHash = executeCommand(getHashCmd.c_str());
    // Trim whitespace
    commitHash.erase(commitHash.find_last_not_of(" \n\r\t") + 1);

    // Create worktree at detached HEAD
    string worktreeCmd = "cd " + devPath + " && /usr/bin/git worktree add --detach " +
                         prodPath + " " + commitHash + " 2>&1";
    string worktreeOutput = executeCommand(worktreeCmd.c_str());
    if (worktreeOutput.find("fatal") != string::npos) {
      result << "  Warning: Could not create worktree: " << worktreeOutput << "\n";
    } else {
      result << "  Worktree: OK\n";
    }
  } else {
    result << "  Prod worktree already exists: " << prodPath << "\n";
  }

  // 3. Detect app structure
  auto [hasServer, serverSubdir, clientSubdir] = detectAppStructure(devPath);

  // Allow overrides from command
  if (command.contains(COMMAND_ARG_HAS_SERVER)) {
    hasServer = command[COMMAND_ARG_HAS_SERVER].get<bool>();
  }
  if (command.contains(COMMAND_ARG_SERVER_SUBDIR)) {
    serverSubdir = command[COMMAND_ARG_SERVER_SUBDIR].get<string>();
    if (!serverSubdir.empty()) hasServer = true;
  }
  if (command.contains(COMMAND_ARG_CLIENT_SUBDIR)) {
    clientSubdir = command[COMMAND_ARG_CLIENT_SUBDIR].get<string>();
  }

  result << "  Structure detected:\n";
  result << "    Has server: " << (hasServer ? "yes" : "no") << "\n";
  if (hasServer) {
    result << "    Server subdir: " << serverSubdir << "\n";
  }
  result << "    Client subdir: " << (clientSubdir.empty() ? "(root)" : clientSubdir) << "\n";

  // 4. Allocate ports
  auto [prodPort, devPort] = findNextAvailablePortPair();
  if (prodPort < 0) {
    return CmdResult(1, "No available ports in 3000-3499 range\n");
  }

  int serverPort = -1;
  if (hasServer) {
    serverPort = findNextAvailableServerPort();
    if (serverPort < 0) {
      return CmdResult(1, "No available server ports in 3500+ range\n");
    }
  }

  // Save ports to database
  SettingsTable::setSetting("port_" + appId + "-prod", to_string(prodPort));
  SettingsTable::setSetting("port_" + appId + "-dev", to_string(devPort));
  if (hasServer) {
    SettingsTable::setSetting("port_" + appId + "-server", to_string(serverPort));
  }

  result << "  Ports allocated:\n";
  result << "    " << appId << "-prod: " << prodPort << "\n";
  result << "    " << appId << "-dev: " << devPort << "\n";
  if (hasServer) {
    result << "    " << appId << "-server: " << serverPort << "\n";
  }

  // 5. Create app record
  ExtraAppRecord appRecord;
  appRecord.app_id = appId;
  appRecord.display_name = command.contains(COMMAND_ARG_DISPLAY_NAME)
      ? command[COMMAND_ARG_DISPLAY_NAME].get<string>()
      : appId;
  appRecord.repo_url = repoUrl;
  appRecord.has_server_component = hasServer;
  appRecord.server_service_template = hasServer ? (appId + "-server-{mode}") : "";
  appRecord.client_service_template = appId + "-{mode}";
  appRecord.port_key_client = appId;
  appRecord.port_key_server = hasServer ? (appId + "-server") : "";
  appRecord.dev_path = devPath;
  appRecord.prod_path = prodPath;
  appRecord.server_build_subdir = serverSubdir;
  appRecord.client_subdir = clientSubdir;

  ExtraAppTable::upsertApp(appRecord);
  result << "  Database entry: OK\n";

  // 6. Fix permissions
  string fixPermsCmd = "fixAutoLinuxPerms " + devPath + " 2>/dev/null";
  std::system(fixPermsCmd.c_str());
  fixPermsCmd = "fixAutoLinuxPerms " + prodPath + " 2>/dev/null";
  std::system(fixPermsCmd.c_str());

  result << "\nApp added successfully!\n";
  result << "Next steps:\n";
  result << "  1. Install dependencies: d installAppDeps --app " << appId << "\n";
  if (hasServer) {
    result << "  2. Build server: d buildApp --app " << appId << " --mode dev\n";
  }
  result << "  3. Create systemd services for " << appId << "-dev and " << appId << "-prod\n";
  result << "  4. Start app: d startApp --app " << appId << " --mode dev\n";

  return CmdResult(0, result.str());
}

CmdResult handleRemoveExtraApp(const json &command) {
  if (!command.contains(COMMAND_ARG_APP)) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command[COMMAND_ARG_APP].get<string>();

  if (!ExtraAppTable::appExists(appId)) {
    return CmdResult(1, "App not found in database: " + appId + "\n");
  }

  stringstream result;
  result << "Removing extra app: " << appId << "\n";

  // Get app info before deleting
  ExtraAppRecord app = ExtraAppTable::getApp(appId);

  // Delete port assignments
  SettingsTable::deleteSetting("port_" + appId + "-prod");
  SettingsTable::deleteSetting("port_" + appId + "-dev");
  if (app.has_server_component) {
    SettingsTable::deleteSetting("port_" + appId + "-server");
  }
  result << "  Removed port assignments\n";

  // Delete from database
  ExtraAppTable::deleteApp(appId);
  result << "  Removed database entry\n";

  result << "\nApp removed from registry.\n";
  result << "Note: Files at " << app.dev_path << " and " << app.prod_path << " were NOT deleted.\n";
  result << "To fully remove, manually delete these directories and any systemd services.\n";

  return CmdResult(0, result.str());
}

CmdResult handleProdStatus(const json &command) {
  if (!command.contains(COMMAND_ARG_APP)) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command[COMMAND_ARG_APP].get<string>();
  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  if (config.prodPath.empty()) {
    return CmdResult(1, "App " + appId + " has no prod path configured\n");
  }

  stringstream result;
  result << "Prod status for " << appId << ":\n";
  result << "  Path: " << config.prodPath << "\n";

  // Get current commit
  string commitCmd =
      "cd " + config.prodPath + " && /usr/bin/git rev-parse --short HEAD 2>&1";
  string commit = executeCommand(commitCmd.c_str());
  commit.erase(commit.find_last_not_of(" \n\r\t") + 1);
  result << "  Commit: " << commit << "\n";

  // Check for uncommitted changes
  string statusCmd =
      "cd " + config.prodPath + " && /usr/bin/git status --porcelain 2>&1";
  string statusOutput = executeCommand(statusCmd.c_str());

  if (statusOutput.empty()) {
    result << "  Status: CLEAN\n";
  } else {
    result << "  Status: DIRTY (has uncommitted changes)\n";
    result << "  Changes:\n";
    // Indent each line
    stringstream ss(statusOutput);
    string line;
    while (getline(ss, line)) {
      result << "    " << line << "\n";
    }
  }

  return CmdResult(0, result.str());
}

CmdResult handleCleanProd(const json &command) {
  if (!command.contains(COMMAND_ARG_APP)) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command[COMMAND_ARG_APP].get<string>();
  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  if (config.prodPath.empty()) {
    return CmdResult(1, "App " + appId + " has no prod path configured\n");
  }

  stringstream result;
  result << "Cleaning prod for " << appId << "...\n";

  // Discard all local changes
  string cleanCmd = "cd " + config.prodPath +
                    " && /usr/bin/git checkout -- . && /usr/bin/git clean -fd 2>&1";
  string cleanOutput = executeCommand(cleanCmd.c_str());

  if (cleanOutput.find("fatal") != string::npos ||
      cleanOutput.find("error") != string::npos) {
    return CmdResult(1, "Failed to clean prod:\n" + cleanOutput + "\n");
  }

  result << "  Discarded uncommitted changes\n";
  result << "Prod is now clean.\n";

  return CmdResult(0, result.str());
}

CmdResult handleDeployToProd(const json &command) {
  if (!command.contains(COMMAND_ARG_APP)) {
    return CmdResult(1, "Missing required argument: --app\n");
  }

  string appId = command[COMMAND_ARG_APP].get<string>();
  string commit = command.contains(COMMAND_ARG_COMMIT)
                      ? command[COMMAND_ARG_COMMIT].get<string>()
                      : "";

  AppConfig config = AppManager::getAppConfig(appId);
  if (config.appId.empty()) {
    return CmdResult(1, "Unknown app: " + appId + "\n");
  }

  if (config.prodPath.empty()) {
    return CmdResult(1, "App " + appId + " has no prod path configured\n");
  }

  stringstream result;
  result << "Deploying " << appId << " to prod...\n";

  // 1. If no commit specified, get latest from dev
  if (commit.empty()) {
    string getHashCmd =
        "cd " + config.devPath + " && /usr/bin/git rev-parse HEAD 2>&1";
    commit = executeCommand(getHashCmd.c_str());
    // Trim whitespace
    commit.erase(commit.find_last_not_of(" \n\r\t") + 1);
    if (commit.empty() || commit.find("fatal") != string::npos) {
      return CmdResult(1, "Failed to get commit from dev: " + commit + "\n");
    }
    result << "  Using latest dev commit: " << commit.substr(0, 7) << "\n";
  } else {
    result << "  Using specified commit: " << commit.substr(0, 7) << "\n";
  }

  // 2. Check prod worktree status - must be clean
  string statusCmd =
      "cd " + config.prodPath + " && /usr/bin/git status --porcelain 2>&1";
  string statusOutput = executeCommand(statusCmd.c_str());
  if (!statusOutput.empty() && statusOutput.find("fatal") == string::npos) {
    return CmdResult(
        1, "Prod worktree has uncommitted changes. Aborting.\n"
           "Run: git -C " +
               config.prodPath + " status\n");
  }

  // 3. Checkout the commit in prod
  string checkoutCmd = "cd " + config.prodPath +
                       " && /usr/bin/git checkout " + commit + " 2>&1";
  string checkoutOutput = executeCommand(checkoutCmd.c_str());
  if (checkoutOutput.find("fatal") != string::npos ||
      checkoutOutput.find("error") != string::npos) {
    return CmdResult(1, "Failed to checkout commit:\n" + checkoutOutput + "\n");
  }
  result << "  Checkout: OK\n";

  // 4. Rebuild server if app has one
  if (config.hasServerComponent) {
    result << "  Rebuilding server...\n";
    string buildResult = AppManager::buildServerComponent(appId, "prod");
    if (buildResult.find("Build complete") == string::npos) {
      return CmdResult(1, "Build failed:\n" + buildResult);
    }
    result << "  Build: OK\n";
  }

  // 5. Restart prod services
  result << "  Restarting services...\n";

  // Stop first
  if (!config.clientServiceTemplate.empty()) {
    string clientService = AppManager::resolveServiceName(
        config.clientServiceTemplate, appId, "prod");
    AppManager::stopService(clientService);
  }
  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService = AppManager::resolveServiceName(
        config.serverServiceTemplate, appId, "prod");
    AppManager::stopService(serverService);
  }

  // Wait for ports
  string portKey = "port_" + config.portKeyClient + "-prod";
  string portStr = SettingsTable::getSetting(portKey);
  if (!portStr.empty()) {
    AppManager::waitForPortRelease(stoi(portStr));
  }

  // Start services
  if (config.hasServerComponent && !config.serverServiceTemplate.empty()) {
    string serverService = AppManager::resolveServiceName(
        config.serverServiceTemplate, appId, "prod");
    AppManager::startService(serverService);
    result << "  Started " << serverService << "\n";
  }
  if (!config.clientServiceTemplate.empty()) {
    string clientService = AppManager::resolveServiceName(
        config.clientServiceTemplate, appId, "prod");
    AppManager::startService(clientService);
    result << "  Started " << clientService << "\n";
  }

  result << "\nDeploy complete!\n";
  return CmdResult(0, result.str());
}

CmdResult handleListExtraApps(const json &) {
  vector<ExtraAppRecord> apps = ExtraAppTable::getAllApps();

  if (apps.empty()) {
    return CmdResult(0, "No extra apps registered in database.\n");
  }

  stringstream ss;
  ss << "Extra Apps (from database):\n";
  ss << "==========================\n";

  for (const auto &app : apps) {
    ss << "\n" << app.app_id << ":\n";
    ss << "  Display Name: " << app.display_name << "\n";
    ss << "  Repo URL: " << app.repo_url << "\n";
    ss << "  Dev Path: " << app.dev_path << "\n";
    ss << "  Prod Path: " << app.prod_path << "\n";
    ss << "  Has Server: " << (app.has_server_component ? "yes" : "no") << "\n";
    if (app.has_server_component) {
      ss << "  Server Subdir: " << app.server_build_subdir << "\n";
    }
    ss << "  Client Subdir: " << (app.client_subdir.empty() ? "(root)" : app.client_subdir) << "\n";

    // Show port assignments
    string prodPort = SettingsTable::getSetting("port_" + app.app_id + "-prod");
    string devPort = SettingsTable::getSetting("port_" + app.app_id + "-dev");
    ss << "  Ports: prod=" << prodPort << ", dev=" << devPort;
    if (app.has_server_component) {
      string serverPort = SettingsTable::getSetting("port_" + app.app_id + "-server");
      ss << ", server=" << serverPort;
    }
    ss << "\n";
  }

  return CmdResult(0, ss.str());
}
