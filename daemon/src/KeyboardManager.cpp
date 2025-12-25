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
  FILE *pipe;
  string output;
  int status;
  int exitCode;
  logMessage = string("[START setKeyboard]\n");
  logToFile(logMessage);
  string scriptPath =
      directories.mappings + PREFIX_KEYBOARD + ALL_KEYBOARD + ".sh";
  string scriptContent = readScriptFile(scriptPath);
  if (scriptContent.empty()) {
    return CmdResult(1, "Script file not found\n");
  }
  scriptContent = substituteVariable(scriptContent, KEYBOARD_PATH_KEY, kvTable.get(KEYBOARD_PATH_KEY));
  scriptContent = substituteVariable(scriptContent, MOUSE_PATH_KEY, kvTable.get(MOUSE_PATH_KEY));
  scriptContent = substituteVariable(scriptContent, EVSIEVE_RANDOM_VAR, to_string(rand() % 1000000));
  scriptContent = substituteVariable(scriptContent, CODE_FOR_APP_CODES, VALUE_FOR_APP_CODES);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_DEFAULT, VALUE_FOR_DEFAULT);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_CODE, VALUE_FOR_CODE);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_GNOME_TERMINAL, VALUE_FOR_GNOME_TERMINAL);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_GOOGLE_CHROME, VALUE_FOR_GOOGLE_CHROME);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_CNTRL_V, VALUE_FOR_CNTRL_V);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G1, VALUE_FOR_G1);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G2, VALUE_FOR_G2);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G3, VALUE_FOR_G3);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G4, VALUE_FOR_G4);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G5, VALUE_FOR_G5);
  scriptContent = substituteVariable(scriptContent, CODE_FOR_G6, VALUE_FOR_G6);
  string cmd = string("sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; "
    "sudo systemd-run --collect --service-type=simple "
    "--unit=corsairKeyBoardLogiMouse.service "
    "--property=StandardError=append:" +
    // directories.data + EVSIEVE_STANDARD_ERR_FILE + " " +
    directories.data + EVSIEVE_STANDARD_OUTPUT_FILE + " " +
    "--property=StandardOutput=append:" + directories.data +
    EVSIEVE_STANDARD_OUTPUT_FILE + " ") +
    scriptContent;
  if (enableKeyboard) {
    // Enable keyboard: run full setup script
  } else {
    cmd = string("sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; ");
  }
  pipe = popen(cmd.c_str(), "r");
  logMessage = string("[EXEC] ") + cmd + "\n";
  logToFile(logMessage);
  if (!pipe) {
    logMessage = string("[ERROR] popen failed\n");
    logToFile(logMessage);
    return CmdResult(1, "Failed to execute\n");
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  status = pclose(pipe);
  exitCode = WEXITSTATUS(status);
  logMessage = string("[OUTPUT]\n") + output + "\n";
  logToFile(logMessage);
  logMessage = string("[STATUS] raw status: ") + to_string(status) +
               " exit code: " + to_string(exitCode) + "\n";
  logToFile(logMessage);
  if (status != 0) {
    logMessage = string("[END] FAILED\n");
    logToFile(logMessage);
    return CmdResult(1, string("Failed to execute (exit code ") +
                            std::to_string(exitCode) + ", output: " + output +
                            ")\n");
  }
  logMessage = string("[END] SUCCESS\n");
  logToFile(logMessage);
  if (enableKeyboard) {
    return CmdResult(0, string("SUCCESS\n" + output + "Set keyboard to: ") +
                          ALL_KEYBOARD + "\n");
  } else {
    return CmdResult(0, string("SUCCESS\n" + output + "Keyboard disabled\n"));
  }
}
