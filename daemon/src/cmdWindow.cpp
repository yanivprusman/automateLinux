#include "cmdWindow.h"
#include "AutomationManager.h"
#include "Constants.h"
#include "KeyboardManager.h"
#include "Utils.h"
#include <cstring>
#include <mutex>
#include <unistd.h>

using namespace std;

static int g_windowExtensionSocket = -1;
static std::mutex g_windowExtensionSocketMutex;

// External client socket - set by mainCommand before calling handlers
extern int g_clientSocket;

void setWindowExtensionClientSocket(int socket) {
  std::lock_guard<std::mutex> lock(g_windowExtensionSocketMutex);
  g_windowExtensionSocket = socket;
}

CmdResult handleRegisterWindowExtension(const json &) {
  {
    std::lock_guard<std::mutex> lock(g_windowExtensionSocketMutex);
    g_windowExtensionSocket = g_clientSocket;
  }
  logToFile("[Window Ext] Window Extension registered", LOG_WINDOW);
  return CmdResult(0, std::string(R"({"status":"registered"})") +
                          mustEndWithNewLine);
}

CmdResult handleListWindows(const json &) {
  std::lock_guard<std::mutex> lock(g_windowExtensionSocketMutex);
  if (g_windowExtensionSocket == -1) {
    return CmdResult(1, "Window extension not registered");
  }

  std::string msg =
      std::string(R"({"action":"listWindows"})") + mustEndWithNewLine;
  if (write(g_windowExtensionSocket, msg.c_str(), msg.length()) < 0) {
    g_windowExtensionSocket = -1;
    return CmdResult(1, "Failed to write to extension");
  }

  // Simple read blocking
  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  ssize_t n = read(g_windowExtensionSocket, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    return CmdResult(0, std::string(buffer));
  }
  return CmdResult(1, "No response from extension");
}

CmdResult handleActivateWindow(const json &command) {
  std::lock_guard<std::mutex> lock(g_windowExtensionSocketMutex);
  if (g_windowExtensionSocket == -1) {
    return CmdResult(1, "Window extension not registered");
  }

  json forward = command;
  forward["action"] = "activateWindow";
  std::string msg = forward.dump() + mustEndWithNewLine;

  if (write(g_windowExtensionSocket, msg.c_str(), msg.length()) < 0) {
    g_windowExtensionSocket = -1;
    return CmdResult(1, "Failed to write to extension");
  }

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  ssize_t n = read(g_windowExtensionSocket, buffer, sizeof(buffer) - 1);
  if (n > 0) {
    return CmdResult(0, std::string(buffer));
  }
  return CmdResult(0, "Command sent (no reply)");
}

CmdResult handleActiveWindowChanged(const json &command) {
  return AutomationManager::onActiveWindowChanged(command);
}

CmdResult handleGetActiveContext(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getActiveContextJson().dump());
}
