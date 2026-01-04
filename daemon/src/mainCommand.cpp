#include "mainCommand.h"
#include "AutomationManager.h"
#include "DaemonServer.h"
#include "DatabaseTableManagers.h"
#include "Globals.h"
#include "KeyboardManager.h"
#include "MySQLManager.h"
#include "Utils.h"
#include "common.h"
#include "main.h"
#include "sendKeys.h"
#include "using.h"
#include <algorithm>
#include <iostream>
#include <linux/input-event-codes.h>
#include <string>
#include <thread>
#include <tuple>

using namespace std;

const CommandSignature COMMAND_REGISTRY[] = {
    CommandSignature(COMMAND_EMPTY, {}),
    CommandSignature(COMMAND_HELP_DDASH, {}),
    CommandSignature(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_UPDATE_DIR_HISTORY,
                     {COMMAND_ARG_TTY, COMMAND_ARG_PWD}),
    CommandSignature(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHELL_SIGNAL, {COMMAND_ARG_SIGNAL}),
    CommandSignature(COMMAND_SHOW_TERMINAL_INSTANCE, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHOW_ALL_TERMINAL_INSTANCES, {}),
    CommandSignature(COMMAND_DELETE_ENTRY, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_SHOW_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_DELETE_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_SHOW_DB, {}),
    CommandSignature(COMMAND_PRINT_DIR_HISTORY, {}),
    CommandSignature(COMMAND_UPSERT_ENTRY,
                     {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_PING, {}),
    CommandSignature(COMMAND_GET_KEYBOARD_PATH, {}),
    CommandSignature(COMMAND_GET_MOUSE_PATH, {}),
    CommandSignature(COMMAND_GET_SOCKET_PATH, {}),
    CommandSignature(COMMAND_SET_KEYBOARD,
                     {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS,
                      COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID}),

    CommandSignature(COMMAND_GET_KEYBOARD, {}),
    CommandSignature(COMMAND_GET_KEYBOARD_ENABLED, {}),
    CommandSignature(COMMAND_SHOULD_LOG, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_GET_SHOULD_LOG, {}),
    CommandSignature(COMMAND_TOGGLE_KEYBOARD, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_DISABLE_KEYBOARD, {}),
    CommandSignature(COMMAND_ENABLE_KEYBOARD, {}),
    CommandSignature(COMMAND_GET_DIR, {COMMAND_ARG_DIR_NAME}),
    CommandSignature(COMMAND_GET_FILE, {COMMAND_ARG_FILE_NAME}),
    CommandSignature(COMMAND_QUIT, {}),
    CommandSignature(COMMAND_ACTIVE_WINDOW_CHANGED,
                     {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS,
                      COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID}),
    CommandSignature(COMMAND_SET_ACTIVE_TAB_URL, {COMMAND_ARG_URL}),
    CommandSignature(COMMAND_REGISTER_NATIVE_HOST, {}),
    CommandSignature(COMMAND_FOCUS_CHATGPT, {}),
    CommandSignature(COMMAND_FOCUS_ACK, {}),
    CommandSignature(COMMAND_GET_MACROS, {}),
    CommandSignature(COMMAND_UPDATE_MACROS, {COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_GET_ACTIVE_CONTEXT, {}),
    CommandSignature(COMMAND_GET_EVENT_FILTERS, {}),
    CommandSignature(COMMAND_SET_EVENT_FILTERS, {COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_REGISTER_LOG_LISTENER, {}),
    CommandSignature(COMMAND_TEST_INTEGRITY, {}),
    CommandSignature(COMMAND_SIMULATE_INPUT,
                     {COMMAND_ARG_TYPE, COMMAND_ARG_CODE, COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_ADD_LOG_FILTER, {COMMAND_ARG_ACTION}),
    CommandSignature(COMMAND_REMOVE_LOG_FILTER, {}),
    CommandSignature(COMMAND_LIST_LOG_FILTERS, {}),
    CommandSignature(COMMAND_CLEAR_LOG_FILTERS, {}),
    CommandSignature(COMMAND_EMPTY_DIR_HISTORY_TABLE, {}),
    CommandSignature(COMMAND_GET_PORT, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_SET_PORT, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),

};

const size_t COMMAND_REGISTRY_SIZE =
    sizeof(COMMAND_REGISTRY) / sizeof(COMMAND_REGISTRY[0]);

static int clientSocket = -1;
unsigned int shouldLog = LOG_ALL; // Global state for logging
bool g_keyboardEnabled = true;    // Global state for keyboard enable/disable

// Global state for active tab URL (set by Chrome extension)
static std::mutex g_activeTabUrlMutex;
static std::string g_activeTabUrl = "";
static int g_nativeHostSocket = -1;
static std::mutex g_nativeHostSocketMutex;

static std::pair<unsigned int, std::string>
getCommandLogContext(const std::string &commandName) {
  if (commandName == COMMAND_FOCUS_CHATGPT ||
      commandName == COMMAND_SET_ACTIVE_TAB_URL ||
      commandName == COMMAND_FOCUS_ACK ||
      commandName == COMMAND_REGISTER_NATIVE_HOST) {
    return {LOG_CHROME, "[Chrome]"};
  }
  if (commandName == COMMAND_UPDATE_DIR_HISTORY ||
      commandName == COMMAND_OPENED_TTY || commandName == COMMAND_CLOSED_TTY ||
      commandName == COMMAND_CD_FORWARD || commandName == COMMAND_CD_BACKWARD ||
      commandName == COMMAND_SHOW_TERMINAL_INSTANCE ||
      commandName == COMMAND_SHOW_ALL_TERMINAL_INSTANCES) {
    return {LOG_TERMINAL, "[Terminal]"};
  }
  if (commandName == COMMAND_ACTIVE_WINDOW_CHANGED) {
    return {LOG_WINDOW, "[Window]"};
  }
  return {LOG_NETWORK, "[Network]"};
}

// Function to get active tab URL (called from Utils.cpp)
std::string getActiveTabUrlFromExtension() {
  std::lock_guard<std::mutex> lock(g_activeTabUrlMutex);
  return g_activeTabUrl;
}

// Function to trigger ChatGPT focus in Chrome (called from InputMapper.cpp)
void triggerChromeChatGPTFocus() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  if (g_nativeHostSocket != -1) {
    std::string msg =
        std::string(R"({"action":"focusChatGPT"})") + mustEndWithNewLine;
    ssize_t written = write(g_nativeHostSocket, msg.c_str(), msg.length());
    if (written < 0) {
      logToFile("[Chrome Extension] Write to native host failed (errno=" +
                    std::to_string(errno) + "). Invalidating socket.",
                LOG_CHROME);
      g_nativeHostSocket = -1;
    } else {
      logToFile("[Chrome] Sent focus request to native host (fd=" +
                    std::to_string(g_nativeHostSocket) +
                    ", written=" + std::to_string(written) + " bytes): " + msg,
                LOG_CHROME);
    }
  } else {
    logToFile("[Chrome] Cannot focus ChatGPT: native host not registered",
              LOG_CHROME);
  }
}

static void syncLoggingWithBridge() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  if (g_nativeHostSocket != -1) {
    std::string msg = std::string(R"({"action":"updateMask","mask":)") +
                      std::to_string(shouldLog) + "}" + mustEndWithNewLine;
    write(g_nativeHostSocket, msg.c_str(), msg.length());
  }
}

bool isNativeHostConnected() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  return g_nativeHostSocket != -1;
}

static string errorEntryNotFound(const string &key) {
  return string("Entry not found for key ") + key + mustEndWithNewLine;
}

static const string HELP_MESSAGE =
    "Usage: daemon [OPTIONS] <COMMAND>\n\n"
    "Manage terminal history and database entries.\n\n"
    "Commands:\n"
    "  openedTty               Notify daemon that a terminal has been opened.\n"
    "  closedTty               Notify daemon that a terminal has been closed.\n"
    "  updateDirHistory        Update the directory history for a terminal.\n"
    "  cdForward               Move forward in the directory history.\n"
    "  cdBackward              Move backward in the directory history.\n"
    "  showTerminalInstance    Show the current terminal instance.\n"
    "  deleteEntry             Delete a specific entry from the database by "
    "key.\n"
    "  deleteEntriesByPrefix   Delete all entries with a specific prefix.\n"
    "  showDB                  Display all entries in the database.\n"
    "  ping                    Ping the daemon and receive pong response.\n"
    "  getMousePath            Get the path to the mouse input device.\n"
    "  getSocketPath           Get the path to the daemon's UNIX domain "
    "socket.\n"
    "  setKeyboard             Simulate active window change for debugging.\n"

    "  disableKeyboard         Disable the keyboard.\n"
    "  enableKeyboard          Enable the keyboard.\n"
    "  getKeyboardEnabled      Get whether keyboard is enabled (true/false).\n"
    "  shouldLog               Enable or disable logging (true/false).\n"
    "  toggleKeyboard  Toggle automatic keyboard "
    "switching on window change.\n"
    "  getDir                  Get a daemon directory path by name (base, "
    "data, mappings).\n"
    "  simulateInput           Simulate an input event (type, code, value).\n"
    "  addLogFilter            Add a granular filter for input event logging. "
    "Args: --type --code --value --devicePathRegex --isKeyboard --action "
    "(show/hide).\n"
    "  removeLogFilter         Remove a granular filter for input event "
    "logging. "
    "Args: --type --code --value --devicePathRegex --isKeyboard.\n"
    "  listLogFilters          List all active granular log filters.\n"
    "  clearLogFilters         Clear all granular log filters.\n\n"
    "  getDir                  Get a daemon directory path by name (base, "
    "data, mappings).\n"
    "  getFile                 Get file path by name from data or mapping "
    "directories.\n\n"
    "Options:\n"
    "  --help                  Display this help message.\n"
    "  --json                  Output results in JSON format.\n\n"
    "Examples:\n"
    "  daemon openedTty\n"
    "  daemon cdForward\n"
    "  daemon deleteEntriesByPrefix session_\n"
    "  daemon showDB --json\n";

CmdResult handleHelp(const json &) { return CmdResult(0, HELP_MESSAGE); }

CmdResult handleOpenedTty(const json &command) {
  return Terminal::openedTty(command);
}

CmdResult handleClosedTty(const json &command) {
  return Terminal::closedTty(command);
}

CmdResult handleUpdateDirHistory(const json &command) {
  return Terminal::updateDirHistory(command);
}

CmdResult handleCdForward(const json &command) {
  return Terminal::cdForward(command);
}

CmdResult handleCdBackward(const json &command) {
  return Terminal::cdBackward(command);
}

CmdResult handleShowTerminalInstance(const json &command) {
  return Terminal::showTerminalInstance(command);
}

CmdResult handleShowAllTerminalInstances(const json &command) {
  return Terminal::showAllTerminalInstances(command);
}

CmdResult handleShowEntriesByPrefix(const json &command) {
  string prefix = command[COMMAND_ARG_PREFIX].get<string>();
  // This command is now ambiguous since we have multiple tables.
  // For backward compatibility, let's treat it as a generic setting query if it
  // doesn't match specific logic. Or better, return a message that it's
  // deprecated.
  return CmdResult(
      1, "showEntriesByPrefix is deprecated in the new multi-table schema.\n");
}

CmdResult handleDeleteEntriesByPrefix(const json &command) {
  return CmdResult(1, "deleteEntriesByPrefix is deprecated.\n");
}

CmdResult handleShowDb(const json &) {
  // Show summary of all tables
  std::stringstream ss;
  ss << "--- Terminal History ---\n";
  for (const auto &p : TerminalTable::getAllHistory()) {
    ss << "TTY " << std::get<0>(p) << " Index " << std::get<1>(p) << ": "
       << std::get<2>(p) << "\n";
  }
  ss << "--- Terminal Sessions ---\n";
  for (const auto &p : TerminalTable::getAllSessions()) {
    ss << "TTY " << p.first << " -> Index " << p.second << "\n";
  }
  ss << "--- Device Registry ---\n";
  ss << "Keyboard: " << DeviceTable::getDevicePath("keyboard") << "\n";
  ss << "Mouse: " << DeviceTable::getDevicePath("mouse") << "\n";
  ss << "--- System Settings ---\n";
  ss << "shouldLogState: " << SettingsTable::getSetting("shouldLogState")
     << "\n";

  return CmdResult(0, ss.str());
}

CmdResult handleDeleteEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  int rc = SettingsTable::deleteSetting(key);
  if (rc >= 1) {
    return CmdResult(0, "Entry deleted from settings table\n");
  }
  return CmdResult(1, errorEntryNotFound(key));
}

CmdResult handlePrintDirHistory(const json &) {
  std::stringstream ss;
  ss << "--- Directory History Entries ---\n";
  auto history = TerminalTable::getAllHistory();
  if (history.empty()) {
    ss << "No directory entries found.\n";
  } else {
    for (const auto &t : history) {
      ss << "  TTY " << std::get<0>(t) << " Index " << std::get<1>(t) << ": "
         << std::get<2>(t) << "\n";
    }
  }

  ss << "\n--- Terminal Sessions ---\n";
  auto sessions = TerminalTable::getAllSessions();
  if (sessions.empty()) {
    ss << "No active sessions found.\n";
  } else {
    for (const auto &pair : sessions) {
      ss << "  TTY " << pair.first << ": Index " << pair.second << "\n";
    }
  }

  ss << "\n--- Last Touched Directory ---\n";
  string lastTouchedIndex =
      SettingsTable::getSetting(INDEX_OF_LAST_TOUCHED_DIR_KEY);
  if (!lastTouchedIndex.empty()) {
    ss << "  Index: " << lastTouchedIndex
       << " (lookup disabled due to per-TTY schema)\n";
  } else {
    ss << "No last touched directory recorded.\n";
  }

  return CmdResult(0, ss.str());
}

CmdResult handleUpsertEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = command[COMMAND_ARG_VALUE].get<string>();
  SettingsTable::setSetting(key, value);
  return CmdResult(0, "Entry upserted to settings table\n");
}

CmdResult handleGetEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = SettingsTable::getSetting(key);
  if (value.empty()) {
    return CmdResult(1, "\n");
  }
  return CmdResult(0, value + "\n");
}
CmdResult handlePing(const json &) { return CmdResult(0, "pong\n"); }
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

CmdResult handleGetSocketPath(const json &) {
  return CmdResult(0, SOCKET_PATH + string("\n"));
}

CmdResult handleSetKeyboard(const json &command) {
  return AutomationManager::onActiveWindowChanged(command);
}

CmdResult handleShouldLog(const json &command) {
  unsigned int newLogMask = shouldLog;
  auto val = command[COMMAND_ARG_ENABLE];

  if (val.is_string()) {
    string enableStr = val.get<string>();
    if (enableStr == "true") {
      newLogMask = LOG_ALL;
    } else if (enableStr == "false") {
      newLogMask = LOG_NONE;
    } else {
      try {
        newLogMask = std::stoul(enableStr);
      } catch (...) {
        return CmdResult(1, "Invalid logging value string.\n");
      }
    }
  } else if (val.is_number()) {
    newLogMask = val.get<unsigned int>();
  } else {
    return CmdResult(1, "Invalid logging value type.\n");
  }

  shouldLog = newLogMask;
  SettingsTable::setSetting("shouldLogState", to_string(shouldLog));
  syncLoggingWithBridge();
  return CmdResult(0, string("Logging mask set to: ") + to_string(shouldLog) +
                          "\n");
}

CmdResult handleGetShouldLog(const json &) {
  return CmdResult(0, to_string(shouldLog) + string("\n"));
}

CmdResult handletoggleKeyboard(const json &command) {
  string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
  g_keyboardEnabled = (enableStr == COMMAND_VALUE_TRUE);
  return KeyboardManager::setKeyboard(g_keyboardEnabled);
}

CmdResult handleGetDir(const json &command) {
  string dirName = command[COMMAND_ARG_DIR_NAME].get<string>();
  string result;
  if (dirName == "base") {
    result = directories.base;
  } else if (dirName == "data") {
    result = directories.data;
  } else if (dirName == "mappings") {
    result = directories.mappings;
  } else {
    return CmdResult(1, "Unknown directory name: " + dirName + "\n");
  }
  return CmdResult(0, result + "\n");
}

CmdResult handleGetFile(const json &command) {
  string fileName = command[COMMAND_ARG_FILE_NAME].get<string>();
  for (const auto &file : files.files) {
    if (file.name.find(fileName) != string::npos) {
      return CmdResult(0, file.fullPath() + "\n");
    }
  }
  return CmdResult(1, "File not found: " + fileName + "\n");
}

CmdResult handleQuit(const json &) {
  running = 0; // Signal the daemon to shut down
  return CmdResult(0, "Shutting down daemon.\n");
}

CmdResult handleActiveWindowChanged(const json &command) {
  return AutomationManager::onActiveWindowChanged(command);
}

CmdResult handleSetActiveTabUrl(const json &command) {
  std::string url = command[COMMAND_ARG_URL].get<std::string>();
  {
    std::lock_guard<std::mutex> lock(g_activeTabUrlMutex);
    g_activeTabUrl = url;
  }
  logToFile("[Chrome] Active tab changed to: " + url, LOG_CHROME);
  KeyboardManager::setContext(AppType::CHROME, url, "");
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}

CmdResult handleRegisterNativeHost(const json &) {
  {
    std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
    g_nativeHostSocket = clientSocket;
  }
  logToFile("[Chrome] Native messaging host registered", LOG_CHROME);
  syncLoggingWithBridge();
  return CmdResult(0, std::string(R"({"status":"registered"})") +
                          mustEndWithNewLine);
}

CmdResult handleFocusChatGPT(const json &) {
  triggerChromeChatGPTFocus();
  return CmdResult(0, "Focus request sent\n");
}

CmdResult handleFocusAck(const json &) {
  KeyboardManager::onFocusAck();
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}

CmdResult handleShellSignal(const json &command) {
  string signal = command[COMMAND_ARG_SIGNAL].get<string>();
  // We return a bash command that the shell will eval.
  // This allows the shell to signal itself (e.g. SIGWINCH)
  // after the bind -x call completes.
  string bashCmd = "kill -" + signal + " $$" + mustEndWithNewLine;
  return CmdResult(0, bashCmd);
}

CmdResult handleDisableKeyboard(const json &) {
  g_keyboardEnabled = false;
  return KeyboardManager::setKeyboard(false);
}

CmdResult handleEnableKeyboard(const json &) {
  g_keyboardEnabled = true;
  return KeyboardManager::setKeyboard(true);
}

CmdResult handleGetMacros(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getMacrosJson().dump());
}

CmdResult handleGetActiveContext(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getActiveContextJson().dump());
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

CmdResult handleGetEventFilters(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getEventFiltersJson().dump());
}

CmdResult handleSetEventFilters(const json &command) {
  try {
    json j = json::parse(command[COMMAND_ARG_VALUE].get<string>());
    KeyboardManager::mapper.setEventFilters(j);
    return CmdResult(0, "Event filters updated successfully");
  } catch (const std::exception &e) {
    return CmdResult(1, std::string("Failed to parse filters: ") + e.what());
  }
}

CmdResult handleRegisterLogListener(const json &) {
  registerLogSubscriber(clientSocket);
  return CmdResult(0, "Subscribed to logs\n");
}

CmdResult handleGetKeyboard(const json &) {
  return CmdResult(
      0, (g_keyboardEnabled ? COMMAND_VALUE_TRUE : COMMAND_VALUE_FALSE) +
             string("\n"));
}

CmdResult handleTestIntegrity(const json &) {
  // Execute the loopback test script interactively
  FILE *pipe = popen(
      "python3 /home/yaniv/coding/automateLinux/test-input-loopback.py", "r");
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
  uint16_t type = command[COMMAND_ARG_TYPE].get<uint16_t>();
  uint16_t code = command[COMMAND_ARG_CODE].get<uint16_t>();
  int32_t value = command[COMMAND_ARG_VALUE].get<int32_t>();
  KeyboardManager::mapper.emit(type, code, value);
  KeyboardManager::mapper.sync();
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}

CmdResult handleEmptyDirHistoryTable(const json &) {
  MySQLManager::emptyTable("terminal_history");
  return CmdResult(0, "terminal_history table emptied.\n");
}

CmdResult handleGetPort(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string portKey = "port_" + key;
  string value = SettingsTable::getSetting(portKey);
  if (value.empty()) {
    return CmdResult(1, "Port not set for " + key + "\n");
  }
  return CmdResult(0, value + "\n");
}

CmdResult handleSetPort(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value;
  if (command[COMMAND_ARG_VALUE].is_number()) {
    value = to_string(command[COMMAND_ARG_VALUE].get<long long>());
  } else {
    value = command[COMMAND_ARG_VALUE].get<string>();
  }
  string portKey = "port_" + key;
  SettingsTable::setSetting(portKey, value);
  return CmdResult(0, "Port set for " + key + " to " + value + "\n");
}

typedef CmdResult (*CommandHandler)(const json &);

struct CommandDispatch {
  const char *name;
  CommandHandler handler;
};

static const CommandDispatch COMMAND_HANDLERS[] = {
    {COMMAND_EMPTY, handleHelp},
    {COMMAND_HELP_DDASH, handleHelp},
    {COMMAND_OPENED_TTY, handleOpenedTty},
    {COMMAND_CLOSED_TTY, handleClosedTty},
    {COMMAND_UPDATE_DIR_HISTORY, handleUpdateDirHistory},
    {COMMAND_CD_FORWARD, handleCdForward},
    {COMMAND_CD_BACKWARD, handleCdBackward},
    {COMMAND_SHELL_SIGNAL, handleShellSignal},
    {COMMAND_SHOW_TERMINAL_INSTANCE, handleShowTerminalInstance},
    {COMMAND_SHOW_ALL_TERMINAL_INSTANCES, handleShowAllTerminalInstances},
    {COMMAND_SHOW_ENTRIES_BY_PREFIX, handleShowEntriesByPrefix},
    {COMMAND_DELETE_ENTRIES_BY_PREFIX, handleDeleteEntriesByPrefix},
    {COMMAND_SHOW_DB, handleShowDb},
    {COMMAND_DELETE_ENTRY, handleDeleteEntry},
    {COMMAND_PRINT_DIR_HISTORY, handlePrintDirHistory},
    {COMMAND_UPSERT_ENTRY, handleUpsertEntry},
    {COMMAND_GET_ENTRY, handleGetEntry},
    {COMMAND_PING, handlePing},
    {COMMAND_GET_KEYBOARD_PATH, handleGetKeyboardPath},
    {COMMAND_GET_MOUSE_PATH, handleGetMousePath},
    {COMMAND_GET_SOCKET_PATH, handleGetSocketPath},
    {COMMAND_SET_KEYBOARD, handleSetKeyboard},
    {COMMAND_SET_KEYBOARD, handleSetKeyboard},

    {COMMAND_DISABLE_KEYBOARD, handleDisableKeyboard},
    {COMMAND_ENABLE_KEYBOARD, handleEnableKeyboard},
    {COMMAND_GET_KEYBOARD, handleGetKeyboard},
    {COMMAND_GET_KEYBOARD, handleGetKeyboard},
    {COMMAND_GET_KEYBOARD_ENABLED, handleGetKeyboard},
    {COMMAND_SHOULD_LOG, handleShouldLog},
    {COMMAND_GET_SHOULD_LOG, handleGetShouldLog},
    {COMMAND_TOGGLE_KEYBOARD, handletoggleKeyboard},
    {COMMAND_GET_DIR, handleGetDir},
    {COMMAND_GET_FILE, handleGetFile},
    {COMMAND_QUIT, handleQuit},
    {COMMAND_ACTIVE_WINDOW_CHANGED, handleActiveWindowChanged},
    {COMMAND_SET_ACTIVE_TAB_URL, handleSetActiveTabUrl},
    {COMMAND_REGISTER_NATIVE_HOST, handleRegisterNativeHost},
    {COMMAND_FOCUS_CHATGPT, handleFocusChatGPT},
    {COMMAND_FOCUS_ACK, handleFocusAck},
    {COMMAND_GET_MACROS, handleGetMacros},
    {COMMAND_GET_ACTIVE_CONTEXT, handleGetActiveContext},
    {COMMAND_UPDATE_MACROS, handleUpdateMacros},
    {COMMAND_GET_ACTIVE_CONTEXT, handleGetActiveContext},
    {COMMAND_GET_EVENT_FILTERS, handleGetEventFilters},
    {COMMAND_SET_EVENT_FILTERS, handleSetEventFilters},
    {COMMAND_REGISTER_LOG_LISTENER, handleRegisterLogListener},
    {COMMAND_TEST_INTEGRITY, handleTestIntegrity},
    {COMMAND_SIMULATE_INPUT, handleSimulateInput},
    {COMMAND_EMPTY_DIR_HISTORY_TABLE, handleEmptyDirHistoryTable},
    {COMMAND_GET_PORT, handleGetPort},
    {COMMAND_SET_PORT, handleSetPort},

};

static const size_t COMMAND_HANDLERS_SIZE =
    sizeof(COMMAND_HANDLERS) / sizeof(COMMAND_HANDLERS[0]);

CmdResult validateCommand(const json &command) {
  if (!command.contains(COMMAND_KEY)) {
    return CmdResult(1, "Missing command key");
  }
  string commandName = command[COMMAND_KEY].get<string>();
  const CommandSignature *foundCommand = nullptr;
  for (size_t i = 0; i < COMMAND_REGISTRY_SIZE; ++i) {
    if (COMMAND_REGISTRY[i].name == commandName) {
      foundCommand = &COMMAND_REGISTRY[i];
      break;
    }
  }
  if (!foundCommand) {
    return CmdResult(1, string("Unknown command: ") + commandName +
                            mustEndWithNewLine);
  }
  for (const string &arg : foundCommand->requiredArgs) {
    if (!command.contains(arg)) {
      return CmdResult(1, string("Missing required arg: ") + arg +
                              mustEndWithNewLine);
    }
  }
  return CmdResult(0, "");
}

int mainCommand(const json &command, int client_sock) {
  clientSocket = client_sock;
  string commandStr = command.dump();
  string commandName =
      command.contains(COMMAND_KEY) ? command[COMMAND_KEY].get<string>() : "";
  auto logCtx = getCommandLogContext(commandName);
  logToFile(logCtx.second + " Received command: " + commandStr, logCtx.first);
  CmdResult result;
  try {
    CmdResult integrityCheck = validateCommand(command);
    if (integrityCheck.status != 0) {
      result = integrityCheck;
    } else {
      string commandName = command[COMMAND_KEY].get<string>();
      CommandHandler handler = nullptr;
      for (size_t i = 0; i < COMMAND_HANDLERS_SIZE; ++i) {
        if (COMMAND_HANDLERS[i].name == commandName) {
          handler = COMMAND_HANDLERS[i].handler;
          break;
        }
      }
      if (handler) {
        result = handler(command);
      } else {
        result.status = 1;
        result.message =
            string("Unhandled command: ") + commandStr + mustEndWithNewLine;
      }
    }
  } catch (const std::exception &e) {
    result.status = 1;
    result.message = std::string("error: ") + e.what() + "\n";
  }
  if (!result.message.empty() && result.message.back() != '\n') {
    result.message += "\n";
  }
  write(client_sock, result.message.c_str(), result.message.length());

  if (command[COMMAND_KEY] == "closedTty") {
    return 1;
  }

  // Return 1 (close) for regular commands, 0 (keep) for log listeners.
  if (commandName == COMMAND_REGISTER_LOG_LISTENER) {
    return 0;
  }
  return 1;
}