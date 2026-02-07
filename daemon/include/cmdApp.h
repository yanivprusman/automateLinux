#ifndef CMD_APP_H
#define CMD_APP_H

#include "Types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

// App configuration structure
struct AppConfig {
  std::string appId;
  std::string displayName;
  bool hasServerComponent;
  std::string serverServiceTemplate; // e.g., "{app}-server-{mode}"
  std::string clientServiceTemplate; // e.g., "{app}-client-{mode}"
  std::string portKeyClient;         // e.g., "cad" -> port_cad-dev
  std::string portKeyServer;         // e.g., "cad-server" -> port_cad-server
  std::string devPath;               // e.g., "/opt/dev/cad"
  std::string prodPath;              // e.g., "/opt/prod/cad"
  std::string serverBuildSubdir;     // e.g., "server" (relative to app path)
  std::string clientSubdir;          // e.g., "client" (relative to app path)
};

// Command handlers
CmdResult handleStartApp(const json &command);
CmdResult handleStopApp(const json &command);
CmdResult handleRestartApp(const json &command);
CmdResult handleAppStatus(const json &command);
CmdResult handleListApps(const json &command);
CmdResult handleBuildApp(const json &command);
CmdResult handleInstallAppDeps(const json &command);
CmdResult handleEnableApp(const json &command);
CmdResult handleDisableApp(const json &command);
CmdResult handleAddExtraApp(const json &command);
CmdResult handleRemoveExtraApp(const json &command);
CmdResult handleListExtraApps(const json &command);
CmdResult handleDeployToProd(const json &command);
CmdResult handleProdStatus(const json &command);
CmdResult handleCleanProd(const json &command);
CmdResult handleGetAppPeers(const json &command);
CmdResult handleInstallAppOnPeer(const json &command);
CmdResult handleUninstallAppOnPeer(const json &command);
CmdResult handleStartAppOnPeer(const json &command);
CmdResult handleStopAppOnPeer(const json &command);
CmdResult handleInstallAppServices(const json &command);

// App Manager namespace for internal utilities
namespace AppManager {

// Service control (direct systemctl calls)
bool startService(const std::string &serviceName);
bool stopService(const std::string &serviceName);
bool restartService(const std::string &serviceName);
bool enableService(const std::string &serviceName);
bool disableService(const std::string &serviceName);
bool isServiceActive(const std::string &serviceName);
bool isServiceEnabled(const std::string &serviceName);
std::string getServiceStatus(const std::string &serviceName);

// Port management
bool isPortListening(int port);
bool waitForPortRelease(int port, int timeoutMs = 10000);
void killProcessOnPort(int port);

// App configuration
AppConfig getAppConfig(const std::string &appId);
std::vector<AppConfig> getAllApps();

// Service name resolution: "{app}-server-{mode}" -> "cad-server-dev"
std::string resolveServiceName(const std::string &templateStr,
                               const std::string &appId,
                               const std::string &mode);

// Path resolution
std::string getAppPath(const std::string &appId, const std::string &mode);

// Build helpers
std::string buildServerComponent(const std::string &appId,
                                 const std::string &mode);
std::string buildCppComponent(const std::string &path);

// Dependency installation
std::string installDependencies(const std::string &appId,
                                const std::string &mode,
                                const std::string &component);

// Peer app status cache (leader-side, populated from worker heartbeats)
json getLocalAppStatusAll();
void updatePeerAppStatus(const std::string &peerId, const json &appStatus);
void clearPeerAppStatus(const std::string &peerId);

} // namespace AppManager

#endif // CMD_APP_H
