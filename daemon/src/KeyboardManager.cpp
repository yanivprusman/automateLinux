#include "KeyboardManager.h"
#include "Constants.h"
#include "Globals.h"
#include "Utils.h"
#include "main.h" // For KVTable definition
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
  string logMessage;
  logMessage = string("[START setKeyboard] ") +
               (enableKeyboard ? "Enable" : "Disable") + "\n";
  logToFile(logMessage);

  if (enableKeyboard) {
    if (mapper.isRunning()) {
      std::cerr << "Keyboard already enabled" << std::endl;
      return CmdResult(0, "Keyboard already enabled\n");
    }
    std::string keyboardPath = kvTable.get(KEYBOARD_PATH_KEY);
    std::string mousePath = kvTable.get(MOUSE_PATH_KEY);

    if (keyboardPath.empty()) {
      std::cerr << "ERROR: Keyboard path empty" << std::endl;
      return CmdResult(1, "Keyboard path not found\n");
    }
    std::cerr << "Enabling keyboard: " << keyboardPath << std::endl;
    if (mapper.start(keyboardPath, mousePath)) {
      return CmdResult(0, "Keyboard enabled\n");
    } else {
      std::cerr << "ERROR: Failed to start InputMapper" << std::endl;
      return CmdResult(1, "Failed to enable keyboard\n");
    }
  } else {
    std::cerr << "Disabling keyboard" << std::endl;
    mapper.stop();
    return CmdResult(0, "Keyboard disabled\n");
  }
}

void KeyboardManager::setContext(const std::string &appName,
                                 const std::string &url,
                                 const std::string &title) {
  mapper.setContext(appName, url, title);
}
