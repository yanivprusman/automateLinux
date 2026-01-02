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

CmdResult AutomationManager::onActiveWindowChanged(const json &command) {
  logToFile("AutomationManager: onActiveWindowChanged called!\n", LOG_WINDOW);
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
  logToFile("AutomationManager: " + logMessage, LOG_WINDOW);

  std::string wmClass = command[COMMAND_ARG_WM_CLASS].get<string>();
  std::string windowTitle = command[COMMAND_ARG_WINDOW_TITLE].get<string>();

  logToFile("[ACTIVE_WINDOW_CHANGED] Received wmClass: [" + wmClass +
                "] Title: [" + windowTitle + "]\n",
            LOG_WINDOW);
  std::string url = "";
  if (wmClass == wmClassChrome || wmClass == wmClassAntigravity) {
    // Pass windowTitle to help disambiguate correct tab
    url = getChromeTabUrl(windowTitle);
    logToFile(
        "[ACTIVE_WINDOW_CHANGED] Chrome/Antigravity detected. Current URL: [" +
            url + "]\n",
        LOG_WINDOW);
  }

  AppType appType = stringToAppType(wmClass);

  // Fallback: Check title for Antigravity or ChatGPT to ensure Chrome macros
  // work in dev
  std::string lowerTitle = windowTitle;
  std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(),
                 ::tolower);
  if (appType == AppType::OTHER &&
      (lowerTitle.find("antigravity") != std::string::npos ||
       lowerTitle.find("chatgpt") != std::string::npos)) {
    appType = AppType::CHROME;
  }
  if (appType == AppType::OTHER &&
      (lowerTitle.find("terminal") != std::string::npos)) {
    appType = AppType::TERMINAL;
  }

  // Set the mapping context directly in InputMapper, tracking title too
  KeyboardManager::setContext(appType, url, windowTitle);

  return CmdResult(0, "Active window info received and mapping updated.\n");
}
