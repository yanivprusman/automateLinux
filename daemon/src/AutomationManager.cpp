#include "AutomationManager.h"
#include "Constants.h"
#include "Globals.h"         // for g_logFile
#include "KeyboardManager.h" // Added include
#include "Utils.h"
#include "sendKeys.h"
#include <iostream>

using std::string;
using std::to_string;

extern int g_keyboard_fd; // From main.cpp

std::string AutomationManager::getCurrentTabUrl() {
  std::string response = httpGet("http://localhost:9222/json");
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(response, root)) {
    return "";
  }
  for (const auto &tab : root) {
    std::string type = tab["type"].asString();
    std::string url = tab["url"].asString();
    // Skip over extension pages and devtools
    if (type == "page" &&
        url.find("chrome-extension://") == std::string::npos &&
        url.find("devtools://") == std::string::npos) {
      return url;
    }
  }
  return "";
}

CmdResult AutomationManager::onActiveWindowChanged(const json &command) {
  std::cerr << "AutomationManager: onActiveWindowChanged called!" << std::endl;
  string logMessage = "[ACTIVE_WINDOW_CHANGED] ";
  logMessage +=
      "windowTitle: " + command[COMMAND_ARG_WINDOW_TITLE].get<string>() + ", ";
  logMessage +=
      "wmClass: " + command[COMMAND_ARG_WM_CLASS].get<string>() + ", ";
  logMessage +=
      "wmInstance: " + command[COMMAND_ARG_WM_INSTANCE].get<string>() + ", ";
  logMessage += "windowId: " +
                std::to_string(command[COMMAND_ARG_WINDOW_ID].get<long>()) +
                "\n";
  std::cerr << "AutomationManager: " << logMessage << std::flush;
  logToFile(logMessage);

  std::string wmClass = command[COMMAND_ARG_WM_CLASS].get<string>();
  forceLog("[ACTIVE_WINDOW_CHANGED] Received wmClass: [" + wmClass + "]");
  std::string url = "";
  if (wmClass == wmClassChrome) {
    url = getCurrentTabUrl();
    forceLog("[ACTIVE_WINDOW_CHANGED] Chrome detected. Current URL: [" + url +
             "]");
  }

  // Set the mapping context directly in InputMapper
  KeyboardManager::setContext(wmClass, url);

  return CmdResult(0, "Active window info received and mapping updated.\n");
}
