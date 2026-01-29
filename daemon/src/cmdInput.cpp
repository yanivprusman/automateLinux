#include "cmdInput.h"
#include "AutomationManager.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "KeyNames.h"
#include "KeyboardManager.h"
#include "Utils.h"
#include <chrono>
#include <linux/input-event-codes.h>
#include <thread>

using namespace std;

// External globals
extern bool g_keyboardEnabled;

void typeChar(char c) {
  uint16_t code = 0;
  bool shift = false;

  switch (c) {
  case 'a':
    code = KEY_A;
    break;
  case 'b':
    code = KEY_B;
    break;
  case 'c':
    code = KEY_C;
    break;
  case 'd':
    code = KEY_D;
    break;
  case 'e':
    code = KEY_E;
    break;
  case 'f':
    code = KEY_F;
    break;
  case 'g':
    code = KEY_G;
    break;
  case 'h':
    code = KEY_H;
    break;
  case 'i':
    code = KEY_I;
    break;
  case 'j':
    code = KEY_J;
    break;
  case 'k':
    code = KEY_K;
    break;
  case 'l':
    code = KEY_L;
    break;
  case 'm':
    code = KEY_M;
    break;
  case 'n':
    code = KEY_N;
    break;
  case 'o':
    code = KEY_O;
    break;
  case 'p':
    code = KEY_P;
    break;
  case 'q':
    code = KEY_Q;
    break;
  case 'r':
    code = KEY_R;
    break;
  case 's':
    code = KEY_S;
    break;
  case 't':
    code = KEY_T;
    break;
  case 'u':
    code = KEY_U;
    break;
  case 'v':
    code = KEY_V;
    break;
  case 'w':
    code = KEY_W;
    break;
  case 'x':
    code = KEY_X;
    break;
  case 'y':
    code = KEY_Y;
    break;
  case 'z':
    code = KEY_Z;
    break;

  case 'A':
    code = KEY_A;
    shift = true;
    break;
  case 'B':
    code = KEY_B;
    shift = true;
    break;
  case 'C':
    code = KEY_C;
    shift = true;
    break;
  case 'D':
    code = KEY_D;
    shift = true;
    break;
  case 'E':
    code = KEY_E;
    shift = true;
    break;
  case 'F':
    code = KEY_F;
    shift = true;
    break;
  case 'G':
    code = KEY_G;
    shift = true;
    break;
  case 'H':
    code = KEY_H;
    shift = true;
    break;
  case 'I':
    code = KEY_I;
    shift = true;
    break;
  case 'J':
    code = KEY_J;
    shift = true;
    break;
  case 'K':
    code = KEY_K;
    shift = true;
    break;
  case 'L':
    code = KEY_L;
    shift = true;
    break;
  case 'M':
    code = KEY_M;
    shift = true;
    break;
  case 'N':
    code = KEY_N;
    shift = true;
    break;
  case 'O':
    code = KEY_O;
    shift = true;
    break;
  case 'P':
    code = KEY_P;
    shift = true;
    break;
  case 'Q':
    code = KEY_Q;
    shift = true;
    break;
  case 'R':
    code = KEY_R;
    shift = true;
    break;
  case 'S':
    code = KEY_S;
    shift = true;
    break;
  case 'T':
    code = KEY_T;
    shift = true;
    break;
  case 'U':
    code = KEY_U;
    shift = true;
    break;
  case 'V':
    code = KEY_V;
    shift = true;
    break;
  case 'W':
    code = KEY_W;
    shift = true;
    break;
  case 'X':
    code = KEY_X;
    shift = true;
    break;
  case 'Y':
    code = KEY_Y;
    shift = true;
    break;
  case 'Z':
    code = KEY_Z;
    shift = true;
    break;

  case '1':
    code = KEY_1;
    break;
  case '2':
    code = KEY_2;
    break;
  case '3':
    code = KEY_3;
    break;
  case '4':
    code = KEY_4;
    break;
  case '5':
    code = KEY_5;
    break;
  case '6':
    code = KEY_6;
    break;
  case '7':
    code = KEY_7;
    break;
  case '8':
    code = KEY_8;
    break;
  case '9':
    code = KEY_9;
    break;
  case '0':
    code = KEY_0;
    break;

  case '!':
    code = KEY_1;
    shift = true;
    break;
  case '@':
    code = KEY_2;
    shift = true;
    break;
  case '#':
    code = KEY_3;
    shift = true;
    break;
  case '$':
    code = KEY_4;
    shift = true;
    break;
  case '%':
    code = KEY_5;
    shift = true;
    break;
  case '^':
    code = KEY_6;
    shift = true;
    break;
  case '&':
    code = KEY_7;
    shift = true;
    break;
  case '*':
    code = KEY_8;
    shift = true;
    break;
  case '(':
    code = KEY_9;
    shift = true;
    break;
  case ')':
    code = KEY_0;
    shift = true;
    break;

  case '-':
    code = KEY_MINUS;
    break;
  case '_':
    code = KEY_MINUS;
    shift = true;
    break;
  case '=':
    code = KEY_EQUAL;
    break;
  case '+':
    code = KEY_EQUAL;
    shift = true;
    break;

  case '[':
    code = KEY_LEFTBRACE;
    break;
  case '{':
    code = KEY_LEFTBRACE;
    shift = true;
    break;
  case ']':
    code = KEY_RIGHTBRACE;
    break;
  case '}':
    code = KEY_RIGHTBRACE;
    shift = true;
    break;

  case '\\':
    code = KEY_BACKSLASH;
    break;
  case '|':
    code = KEY_BACKSLASH;
    shift = true;
    break;

  case ';':
    code = KEY_SEMICOLON;
    break;
  case ':':
    code = KEY_SEMICOLON;
    shift = true;
    break;
  case '\'':
    code = KEY_APOSTROPHE;
    break;
  case '"':
    code = KEY_APOSTROPHE;
    shift = true;
    break;

  case ',':
    code = KEY_COMMA;
    break;
  case '<':
    code = KEY_COMMA;
    shift = true;
    break;
  case '.':
    code = KEY_DOT;
    break;
  case '>':
    code = KEY_DOT;
    shift = true;
    break;
  case '/':
    code = KEY_SLASH;
    break;
  case '?':
    code = KEY_SLASH;
    shift = true;
    break;

  case '`':
    code = KEY_GRAVE;
    break;
  case '~':
    code = KEY_GRAVE;
    shift = true;
    break;

  case ' ':
    code = KEY_SPACE;
    break;
  case '\n':
    code = KEY_ENTER;
    break;
  case '\t':
    code = KEY_TAB;
    break;

  default:
    // Ignore unknown characters
    return;
  }

  if (code == 0)
    return;

  if (shift) {
    KeyboardManager::mapper.emit(EV_KEY, KEY_LEFTSHIFT, 1);
    KeyboardManager::mapper.sync();
  }

  KeyboardManager::mapper.emit(EV_KEY, code, 1);
  KeyboardManager::mapper.sync();
  KeyboardManager::mapper.emit(EV_KEY, code, 0);
  KeyboardManager::mapper.sync();

  if (shift) {
    KeyboardManager::mapper.emit(EV_KEY, KEY_LEFTSHIFT, 0);
    KeyboardManager::mapper.sync();
  }
}

CmdResult handleGetKeyboardPath(const json &) {
  string path = DeviceTable::getDevicePath("keyboard");
  if (path.empty()) {
    return CmdResult(1, "Keyboard path not found");
  }
  return CmdResult(0, path);
}

CmdResult handleGetMousePath(const json &) {
  string path = DeviceTable::getDevicePath("mouse");
  if (path.empty()) {
    return CmdResult(1, "Mouse path not found");
  }
  return CmdResult(0, path);
}

CmdResult handleSetKeyboard(const json &command) {
  return AutomationManager::onActiveWindowChanged(command);
}

CmdResult handleGetKeyboard(const json &) {
  return CmdResult(
      0, (g_keyboardEnabled ? COMMAND_VALUE_TRUE : COMMAND_VALUE_FALSE) +
             string("\n"));
}

CmdResult handleToggleKeyboard(const json &command) {
  string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
  g_keyboardEnabled = (enableStr == COMMAND_VALUE_TRUE);
  return KeyboardManager::setKeyboard(g_keyboardEnabled);
}

CmdResult handleDisableKeyboard(const json &) {
  // DISABLED: Keyboard grab feature removed - using separate keybinding project
  return CmdResult(0, "Keyboard grab feature disabled\n");
}

CmdResult handleEnableKeyboard(const json &) {
  // DISABLED: Keyboard grab feature removed - using separate keybinding project
  return CmdResult(0, "Keyboard grab feature disabled\n");
}

CmdResult handleGetMacros(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getMacrosJson().dump());
}

CmdResult handleUpdateMacros(const json &command) {
  try {
    json j = json::parse(command[COMMAND_ARG_VALUE].get<string>());
    KeyboardManager::mapper.setMacrosFromJson(j);
    return CmdResult(0, "Macros updated successfully");
  } catch (const std::exception &e) {
    return CmdResult(1, std::string("Failed to parse macros: ") + e.what());
  }
}

CmdResult handleTestIntegrity(const json &) {
  // Execute the loopback test script interactively
  FILE *pipe = popen(
      ("python3 " + directories.base + "test-input-loopback.py").c_str(), "r");
  if (!pipe) {
    return CmdResult(1, "Failed to run diagnostic script");
  }

  char buffer[128];
  string output = "";
  bool readySeen = false;

  // Read output line by line
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    string line = buffer;
    output += line;

    // When Python says READY, we emit the test events
    if (!readySeen && line.find("READY") != string::npos) {
      readySeen = true;

      // Small delay to ensure Python listener is fully grabbed
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      // 1. Enter Key
      KeyboardManager::mapper.emit(EV_KEY, KEY_ENTER, 1);
      KeyboardManager::mapper.sync();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      KeyboardManager::mapper.emit(EV_KEY, KEY_ENTER, 0);
      KeyboardManager::mapper.sync();

      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      // 2. Mouse Click
      KeyboardManager::mapper.emit(EV_KEY, BTN_LEFT, 1);
      KeyboardManager::mapper.sync();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      KeyboardManager::mapper.emit(EV_KEY, BTN_LEFT, 0);
      KeyboardManager::mapper.sync();
    }
  }

  pclose(pipe);
  return CmdResult(0, output + mustEndWithNewLine);
}

CmdResult handleSimulateInput(const json &command) {
  if (command.contains(COMMAND_ARG_STRING)) {
    string str = command[COMMAND_ARG_STRING].get<string>();
    for (char c : str) {
      typeChar(c);
    }
    return CmdResult(0, "");
  }

  if (command.contains(COMMAND_ARG_KEY)) {
    string keyName = command[COMMAND_ARG_KEY].get<string>();
    int keyCode = -1;
    int value = -1; // -1 means press + release

    // Check for suffixes
    string baseName = keyName;
    if (keyName.size() > 4 && keyName.substr(keyName.size() - 4) == "Down") {
      baseName = keyName.substr(0, keyName.size() - 4);
      value = 1;
    } else if (keyName.size() > 2 &&
               keyName.substr(keyName.size() - 2) == "Up") {
      baseName = keyName.substr(0, keyName.size() - 2);
      value = 0;
    }

    if (keyName == "syn") {
      KeyboardManager::mapper.sync();
      return CmdResult(0, "");
    }

    if (keyName == "numlock") {
      KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, 0x45);
      KeyboardManager::mapper.emit(EV_KEY, KEY_NUMLOCK, 1);
      KeyboardManager::mapper.sync();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, 0x45);
      KeyboardManager::mapper.emit(EV_KEY, KEY_NUMLOCK, 0);
      KeyboardManager::mapper.sync();
      return CmdResult(0, "");
    }

    if (keyName == "numlockDown") {
      KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, 0x45);
      KeyboardManager::mapper.emit(EV_KEY, KEY_NUMLOCK, 1);
      KeyboardManager::mapper.sync();
      return CmdResult(0, "");
    }

    if (keyName == "numlockUp") {
      KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, 0x45);
      KeyboardManager::mapper.emit(EV_KEY, KEY_NUMLOCK, 0);
      KeyboardManager::mapper.sync();
      return CmdResult(0, "");
    }

    auto itApp = APP_NAME_TO_CODE.find(keyName);
    if (itApp != APP_NAME_TO_CODE.end()) {
      int appCode = itApp->second;
      for (int i = 0; i < 3; i++) {
        KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, KEY_CODE_FOR_APP_SWITCH);
        KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN,
                                     KEY_CODE_FOR_APP_SWITCH + 100 + i);
        KeyboardManager::mapper.sync();
      }
      KeyboardManager::mapper.emit(EV_MSC, MSC_SCAN, appCode);
      KeyboardManager::mapper.sync();
      return CmdResult(0, "");
    }

    auto it = KEY_NAME_TO_CODE.find(baseName);
    if (it != KEY_NAME_TO_CODE.end()) {
      keyCode = it->second;
    } else {
      return CmdResult(1, "Unknown key name: " + keyName + "\n");
    }

    if (value == -1) {
      // Full click
      KeyboardManager::mapper.emit(EV_KEY, (uint16_t)keyCode, 1);
      KeyboardManager::mapper.sync();
      KeyboardManager::mapper.emit(EV_KEY, (uint16_t)keyCode, 0);
      KeyboardManager::mapper.sync();
    } else {
      KeyboardManager::mapper.emit(EV_KEY, (uint16_t)keyCode, (int32_t)value);
      KeyboardManager::mapper.sync();
    }
    return CmdResult(0, "");
  }

  if (!command.contains(COMMAND_ARG_TYPE) ||
      !command.contains(COMMAND_ARG_CODE) ||
      !command.contains(COMMAND_ARG_VALUE)) {
    return CmdResult(1,
                     "Missing arguments: type, code, value, string, or key\n");
  }

  uint16_t type = command[COMMAND_ARG_TYPE].get<uint16_t>();
  uint16_t code = command[COMMAND_ARG_CODE].get<uint16_t>();
  int32_t value = command[COMMAND_ARG_VALUE].get<int32_t>();

  // Rate limiting for input events to prevent flooding
  static std::chrono::steady_clock::time_point lastEventTime;
  static int eventCount = 0;
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEventTime)
          .count();

  if (elapsed < 10) { // Within 10ms window
    eventCount++;
    if (eventCount > 50) { // More than 50 events in 10ms = too fast
      // Skip non-essential events (mouse move), but always allow key events
      if (type == EV_ABS || type == EV_REL) {
        return CmdResult(0, ""); // Rate limited
      }
    }
  } else {
    eventCount = 1;
    lastEventTime = now;
  }

  if (type == EV_SYN) {
    // Just sync, don't emit the SYN event itself
    KeyboardManager::mapper.sync();
  } else if (type == EV_ABS) {
    // For absolute positioning, don't auto-sync (caller sends SYN_REPORT)
    KeyboardManager::mapper.emitNoSync(type, code, value);
  } else {
    // Standard behavior for keys, buttons, etc.
    KeyboardManager::mapper.emit(type, code, value);
  }
  return CmdResult(0, "");
}
