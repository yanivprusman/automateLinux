#include "KeyboardManager.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "Utils.h"
#include "main.h" // For any global flags
#include <cstdio>
#include <iostream>
#include <vector>

using std::string;
using std::to_string;

InputMapper KeyboardManager::mapper;

const std::vector<std::string> KeyboardManager::KNOWN_KEYBOARDS = {
    CODE_KEYBOARD, GNOME_TERMINAL_KEYBOARD, GOOGLE_CHROME_KEYBOARD,
    DEFAULT_KEYBOARD, TEST_KEYBOARD};

bool KeyboardManager::isKnownKeyboard(const std::string &name) {
  for (const auto &known : KNOWN_KEYBOARDS) {
    if (known == name) {
      return true;
    }
  }
  return false;
}

CmdResult KeyboardManager::setKeyboard(bool enableKeyboard) {
  logToFile(string("[START setKeyboard] ") +
                (enableKeyboard ? "Enable" : "Disable"),
            LOG_CORE);

  if (enableKeyboard) {
    // If mapper is already running (in monitoring mode or grabbed),
    // we just need to ensure pendingGrab_ is set and grab will happen when keys
    // are released.
    if (mapper.isRunning()) {
      logToFile("Mapper already running. Ensuring pendingGrab is set.",
                LOG_CORE);
      mapper.setPendingGrab(true); // Ensure grab happens when keys are released
      return CmdResult(0,
                       "Keyboard enable pending (waiting for key release)\n");
    }

    std::string keyboardPath = DeviceTable::getDevicePath("keyboard");
    std::string mousePath = DeviceTable::getDevicePath("mouse");

    if (keyboardPath.empty()) {
      logToFile("ERROR: Keyboard path empty", LOG_CORE);
      return CmdResult(1, "Keyboard path not found\n");
    }
    logToFile("Enabling keyboard: " + keyboardPath, LOG_CORE);

    // Start mapper in monitoring mode, then set pendingGrab_
    if (mapper.start(keyboardPath,
                     mousePath)) { // start() now just opens devices and sets
                                   // monitoringMode_
      mapper.setPendingGrab(true); // Request a grab once keys are released
      return CmdResult(0,
                       "Keyboard enable pending (waiting for key release)\n");
    } else {
      logToFile("ERROR: Failed to start InputMapper for monitoring", LOG_CORE);
      return CmdResult(1, "Failed to enable keyboard\n");
    }
  } else {
    logToFile("Disabling keyboard", LOG_CORE);
    mapper.stop(); // stop() now handles ungrabbing and freeing devices
    return CmdResult(0, "Keyboard disabled\n");
  }
}

void KeyboardManager::setContext(AppType appType, const std::string &url,
                                 const std::string &title) {
  mapper.setContext(appType, url, title);
}

void KeyboardManager::onFocusAck() { mapper.onFocusAck(); }
