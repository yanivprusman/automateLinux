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

static void logToFile(const string &message) {
  if (g_logFile.is_open()) {
    g_logFile << message;
    g_logFile.flush();
  }
}

CmdResult
KeyboardManager::setKeyboard(const std::string &keyboardNameStr,
                             bool toggleKeyboardsWhenActiveWindowChanges) {
  static string previousKeyboard = "";
  string keyboardName = keyboardNameStr;

  if (keyboardName == previousKeyboard) {
    return CmdResult(0, "Keyboard already set to: " + keyboardName + "\n");
  }

  previousKeyboard = keyboardName;
  string logMessage;
  bool isKnown = false;
  FILE *pipe;
  string output;
  int status;
  int exitCode;

  for (const string &known : KNOWN_KEYBOARDS) {
    if (known == keyboardName) {
      isKnown = true;
      break;
    }
  }

  if (!isKnown) {
    keyboardName = DEFAULT_KEYBOARD;
  }

  logMessage = string("[START setKeyboard] keyboard: ") + keyboardName +
               " isKnown: " + (isKnown ? "true" : "false") + "\n";
  logToFile(logMessage);

  string scriptPath =
      directories.mappings + PREFIX_KEYBOARD + keyboardName + ".sh";
  string scriptContent = readScriptFile(scriptPath);

  if (scriptContent.empty()) {
    return CmdResult(1, "Script file not found\n");
  }

  scriptContent = substituteVariable(scriptContent, KEYBOARD_PATH_KEY,
                                     kvTable.get(KEYBOARD_PATH_KEY));
  scriptContent = substituteVariable(scriptContent, MOUSE_PATH_KEY,
                                     kvTable.get(MOUSE_PATH_KEY));
  scriptContent = substituteVariable(scriptContent, EVSIEVE_RANDOM_VAR,
                                     to_string(rand() % 1000000));

  string cmd = string("sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; "
                      "sudo systemd-run --collect --service-type=notify "
                      "--unit=corsairKeyBoardLogiMouse.service "
                      "--property=StandardError=append:" +
                      directories.data + EVSIEVE_STANDARD_ERR_FILE +
                      " "
                      "--property=StandardOutput=append:" +
                      directories.data + EVSIEVE_STANDARD_OUTPUT_FILE + " ") +
               scriptContent;

  if (!toggleKeyboardsWhenActiveWindowChanges) {
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

  return CmdResult(0, string("SUCCESS\n" + output + "Set keyboard to: ") +
                          keyboardName + "\n");
}
