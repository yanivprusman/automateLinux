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
  logToFile(string("[START setKeyboard] ") +
                (enableKeyboard ? "Enable" : "Disable"),
            LOG_CORE);

  if (enableKeyboard) {
    if (mapper.isRunning()) {
      logToFile("Keyboard already enabled", LOG_CORE);
      return CmdResult(0, "Keyboard already enabled\n");
    }
    std::string keyboardPath = kvTable.get(KEYBOARD_PATH_KEY);
    std::string mousePath = kvTable.get(MOUSE_PATH_KEY);

    if (keyboardPath.empty()) {
      logToFile("ERROR: Keyboard path empty", LOG_CORE);
      return CmdResult(1, "Keyboard path not found\n");
    }
    logToFile("Enabling keyboard: " + keyboardPath, LOG_CORE);
    if (mapper.start(keyboardPath, mousePath)) {
      return CmdResult(0, "Keyboard enabled\n");
    } else {
      logToFile("ERROR: Failed to start InputMapper", LOG_CORE);
      return CmdResult(1, "Failed to enable keyboard\n");
    }
  } else {
    logToFile("Disabling keyboard", LOG_CORE);
    mapper.stop();
    return CmdResult(0, "Keyboard disabled\n");
  }
}

void KeyboardManager::setContext(AppType appType, const std::string &url,
                                 const std::string &title) {
  mapper.setContext(appType, url, title);
}

void KeyboardManager::onFocusAck() { mapper.onFocusAck(); }
