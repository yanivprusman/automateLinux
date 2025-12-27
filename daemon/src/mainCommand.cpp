#include "mainCommand.h"
#include "AutomationManager.h"
#include "DaemonServer.h"
#include "Globals.h"
#include "KeyboardManager.h"
#include "common.h"
#include "main.h"
#include "sendKeys.h"
#include <algorithm>
#include <iostream>
#include <string>

using std::string;
using std::to_string;
using std::vector;

const CommandSignature COMMAND_REGISTRY[] = {
    CommandSignature(COMMAND_EMPTY, {}),
    CommandSignature(COMMAND_HELP_DDASH, {}),
    CommandSignature(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_UPDATE_DIR_HISTORY,
                     {COMMAND_ARG_TTY, COMMAND_ARG_PWD}),
    CommandSignature(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY}),
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
    CommandSignature(COMMAND_SET_KEYBOARD, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_GET_KEYBOARD, {}),
    CommandSignature(COMMAND_GET_KEYBOARD_ENABLED, {}),
    CommandSignature(COMMAND_SHOULD_LOG, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_GET_SHOULD_LOG, {}),
    CommandSignature(COMMAND_TOGGLE_KEYBOARD, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_GET_DIR, {COMMAND_ARG_DIR_NAME}),
    CommandSignature(COMMAND_GET_FILE, {COMMAND_ARG_FILE_NAME}),
    CommandSignature(COMMAND_QUIT, {}),
    CommandSignature(COMMAND_ACTIVE_WINDOW_CHANGED,
                     {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS,
                      COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID}),
    CommandSignature(COMMAND_SET_ACTIVE_TAB_URL, {COMMAND_ARG_URL}),
    CommandSignature(COMMAND_REGISTER_NATIVE_HOST, {}),
    CommandSignature(COMMAND_FOCUS_CHATGPT, {}),
};

const size_t COMMAND_REGISTRY_SIZE =
    sizeof(COMMAND_REGISTRY) / sizeof(COMMAND_REGISTRY[0]);

static int clientSocket = -1;
unsigned int shouldLog = LOG_CORE; // Global state for logging
bool g_keyboardEnabled = true;     // Global state for keyboard enable/disable

// Global state for active tab URL (set by Chrome extension)
static std::mutex g_activeTabUrlMutex;
static std::string g_activeTabUrl = "";
static int g_nativeHostSocket = -1;
static std::mutex g_nativeHostSocketMutex;

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
                LOG_AUTOMATION);
      g_nativeHostSocket = -1;
    } else {
      logToFile("[Chrome Extension] Sent focus request to native host (fd=" +
                    std::to_string(g_nativeHostSocket) +
                    ", written=" + std::to_string(written) + " bytes): " + msg,
                LOG_AUTOMATION);
    }
  } else {
    logToFile(
        "[Chrome Extension] Cannot focus ChatGPT: native host not registered",
        LOG_AUTOMATION);
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

static string
formatEntriesAsText(const vector<std::pair<string, string>> &entries) {
  if (entries.empty()) {
    return "<no entries>\n";
  }
  string result;
  for (const auto &pair : entries) {
    result += pair.first + "|" + pair.second + "\n";
  }
  return result;
}

static string errorEntryNotFound(const string &key) {
  return string("Entry not found for key ") + key + mustEndWithNewLine;
}

static string errorDeleteFailed(const string &prefix) {
  return string("Error deleting entries with prefix: ") + prefix +
         mustEndWithNewLine;
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
    "  setKeyboard             Enable or disable keyboard (true/false).\n"
    "  getKeyboardEnabled      Get whether keyboard is enabled (true/false).\n"
    "  shouldLog               Enable or disable logging (true/false).\n"
    "  toggleKeyboard  Toggle automatic keyboard "
    "switching on window change.\n"
    "  getDir                  Get a daemon directory path by name (base, "
    "data, mappings).\n"
    "  getFile                 Get file path by name from data or mapping "
    "directories.\n\n"
    "  getDir                  Get a daemon directory path by name (base, "
    "data, mappings).\n\n"
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
  auto entries = kvTable.getByPrefix(prefix);
  return CmdResult(0, formatEntriesAsText(entries));
}

CmdResult handleDeleteEntriesByPrefix(const json &command) {
  string prefix = command[COMMAND_ARG_PREFIX].get<string>();
  int rc = kvTable.deleteByPrefix(prefix);
  if (rc == SQLITE_OK) {
    return CmdResult(0, "Entries deleted\n");
  }
  return CmdResult(1, errorDeleteFailed(prefix));
}

CmdResult handleShowDb(const json &) {
  return CmdResult(0, formatEntriesAsText(kvTable.getAll()));
}

CmdResult handleDeleteEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  int rc = kvTable.deleteEntry(key);
  if (rc == SQLITE_OK) {
    return CmdResult(0, "Entry deleted\n");
  }
  return CmdResult(1, errorEntryNotFound(key));
}

CmdResult handlePrintDirHistory(const json &) {
  std::stringstream ss;
  ss << "--- Directory History Entries ---\n";
  vector<std::pair<string, string>> dirs =
      kvTable.getByPrefix(DIR_HISTORY_ENTRY_PREFIX);
  std::sort(dirs.begin(), dirs.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  if (dirs.empty()) {
    ss << "No directory entries found.\n";
  } else {
    for (const auto &pair : dirs) {
      ss << "  " << pair.first << ": " << pair.second << "\n";
    }
  }

  ss << "\n--- TTY Pointers to History Entries ---\n";
  vector<std::pair<string, string>> ptsPointers =
      kvTable.getByPrefix(DIR_HISTORY_POINTER_PREFIX);
  std::sort(ptsPointers.begin(), ptsPointers.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  if (ptsPointers.empty()) {
    ss << "No TTY pointers found.\n";
  } else {
    for (const auto &pair : ptsPointers) {
      ss << "  " << pair.first << ": " << pair.second << "\n";
    }
  }

  ss << "\n--- Last Touched Directory ---\n";
  string lastTouchedIndex = kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
  if (!lastTouchedIndex.empty()) {
    string lastTouchedKey = string(DIR_HISTORY_ENTRY_PREFIX) + lastTouchedIndex;
    string lastTouchedValue = kvTable.get(lastTouchedKey);
    ss << "  Index: " << lastTouchedIndex << ", Path: " << lastTouchedValue
       << "\n";
  } else {
    ss << "No last touched directory recorded.\n";
  }

  return CmdResult(0, ss.str());
}

CmdResult handleUpsertEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = command[COMMAND_ARG_VALUE].get<string>();
  int rc = kvTable.upsert(key, value);
  if (rc == SQLITE_OK) {
    return CmdResult(0, "Entry upserted\n");
  }
  string errorMsg = string("Upsert failed with code ") + to_string(rc) + ": " +
                    sqlite3_errstr(rc) + "\n";
  return CmdResult(rc, errorMsg);
}

CmdResult handleGetEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = kvTable.get(key);
  if (value.empty()) {
    return CmdResult(1, "\n");
  }
  return CmdResult(0, value + "\n");
}
CmdResult handlePing(const json &) { return CmdResult(0, "pong\n"); }
CmdResult handleGetKeyboardPath(const json &) {
  string path = kvTable.get("keyboardPath");
  if (path.empty()) {
    return CmdResult(1, "Keyboard path not found");
  }
  return CmdResult(0, path);
}

CmdResult handleGetMousePath(const json &) {
  string path = kvTable.get("mousePath");
  if (path.empty()) {
    return CmdResult(1, "Mouse path not found");
  }
  return CmdResult(0, path);
}

CmdResult handleGetSocketPath(const json &) {
  return CmdResult(0, SOCKET_PATH + string("\n"));
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
  kvTable.upsert("shouldLogState", to_string(shouldLog));
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
  logToFile("[Chrome Extension] Active tab changed to: " + url, LOG_CORE);
  KeyboardManager::setContext(wmClassChrome, url, "");
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}

CmdResult handleRegisterNativeHost(const json &) {
  {
    std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
    g_nativeHostSocket = clientSocket;
  }
  logToFile("[Chrome Extension] Native messaging host registered", LOG_CORE);
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

CmdResult handleSetKeyboard(const json &command) {
  string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
  g_keyboardEnabled = (enableStr == COMMAND_VALUE_TRUE);
  return KeyboardManager::setKeyboard(g_keyboardEnabled);
}

CmdResult handleGetKeyboard(const json &) {
  return CmdResult(
      0, (g_keyboardEnabled ? COMMAND_VALUE_TRUE : COMMAND_VALUE_FALSE) +
             string("\n"));
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
};

static const size_t COMMAND_HANDLERS_SIZE =
    sizeof(COMMAND_HANDLERS) / sizeof(COMMAND_HANDLERS[0]);

CmdResult testIntegrity(const json &command) {
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
  CmdResult result;
  try {
    CmdResult integrityCheck = testIntegrity(command);
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
  if (command[COMMAND_KEY] == "closedTty") {
    return 0;
  }
  write(client_sock, result.message.c_str(), result.message.length());
  return 0;
}