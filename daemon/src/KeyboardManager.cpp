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
      return CmdResult(0, "Keyboard already enabled\n");
    }
    string keyboardPath = kvTable.get(KEYBOARD_PATH_KEY);
    string mousePath = kvTable.get(MOUSE_PATH_KEY);

    if (keyboardPath.empty()) {
      return CmdResult(1, "Keyboard path not set\n");
    }

    if (mapper.start(keyboardPath, mousePath)) {
      logToFile("[END] SUCCESS\n");
      return CmdResult(0, "Keyboard enabled via libevdev\n");
    } else {
      logToFile("[END] FAILED\n");
      return CmdResult(1, "Failed to start InputMapper\n");
    }
  } else {
    mapper.stop();
    logToFile("[END] SUCCESS\n");
    return CmdResult(0, "Keyboard disabled\n");
  }
}
