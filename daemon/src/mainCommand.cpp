#include "mainCommand.h"
#include "main.h"

const CommandSignature COMMAND_REGISTRY[] = {
    CommandSignature(COMMAND_EMPTY, {}),
    CommandSignature(COMMAND_HELP_DDASH, {}),
    CommandSignature(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_UPDATE_DIR_HISTORY, {COMMAND_ARG_TTY, COMMAND_ARG_PWD}),
    CommandSignature(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHOW_TERMINAL_INSTANCE , {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHOW_ALL_TERMINAL_INSTANCES , {}),
    CommandSignature(COMMAND_DELETE_ENTRY, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_SHOW_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_DELETE_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_SHOW_DB, {}),
    CommandSignature(COMMAND_PRINT_DIR_HISTORY, {}),
    CommandSignature(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_PING, {}),
    CommandSignature(COMMAND_GET_KEYBOARD_PATH, {}),
    CommandSignature(COMMAND_SET_KEYBOARD, {COMMAND_ARG_KEYBOARD_NAME}),
    CommandSignature(COMMAND_SHOULD_LOG, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_TOGGLE_KEYBOARDS_WHEN_ACTIVE_WINDOW_CHANGES, {COMMAND_ARG_ENABLE}),
    CommandSignature(COMMAND_GET_DIR, {COMMAND_ARG_DIR_NAME}),
    CommandSignature(COMMAND_GET_FILE, {COMMAND_ARG_FILE_NAME}),
    CommandSignature(COMMAND_QUIT, {}),
};

const size_t COMMAND_REGISTRY_SIZE = sizeof(COMMAND_REGISTRY) / sizeof(COMMAND_REGISTRY[0]);

static int clientSocket = -1;
static bool shouldLog = false;
static bool toggleKeyboardsWhenActiveWindowChanges = true;


static void logToFile(const string& message) {
    if (!shouldLog) return;
    if (!g_logFile.is_open()) return;
    g_logFile << message;
    g_logFile.flush();
}

static const vector<string> KNOWN_KEYBOARDS = {
    CODE_KEYBOARD,
    GNOME_TERMINAL_KEYBOARD,
    GOOGLE_CHROME_KEYBOARD,
    DEFAULT_KEYBOARD,
    TEST_KEYBOARD
};

static string formatEntriesAsText(const vector<std::pair<string, string>>& entries) {
    if (entries.empty()) {
        return "<no entries>\n";
    }
    string result;
    for (const auto& pair : entries) {
        result += pair.first + "|" + pair.second + "\n";
    }
    return result;
}

static string errorEntryNotFound(const string& key) {
    return string("Entry not found for key ") + key + mustEndWithNewLine;
}

static string errorDeleteFailed(const string& prefix) {
    return string("Error deleting entries with prefix: ") + prefix + mustEndWithNewLine;
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
    "  deleteEntry             Delete a specific entry from the database by key.\n"
    "  deleteEntriesByPrefix   Delete all entries with a specific prefix.\n"
    "  showDB                  Display all entries in the database.\n"
    "  ping                    Ping the daemon and receive pong response.\n"
    "  setKeyboard             Set the keyboard by name and execute restart script.\n"
    "  shouldLog               Enable or disable logging (true/false).\n"
    "  toggleKeyboardsWhenActiveWindowChanges  Toggle automatic keyboard switching on window change.\n"
    "  getDir                  Get a daemon directory path by name (base, data, mappings).\n"
    "  getFile                 Get file path by name from data or mapping directories.\n\n"
    "  getDir                  Get a daemon directory path by name (base, data, mappings).\n\n"
    "Options:\n"
    "  --help                  Display this help message.\n"
    "  --json                  Output results in JSON format.\n\n"
    "Examples:\n"
    "  daemon openedTty\n"
    "  daemon cdForward\n"
    "  daemon deleteEntriesByPrefix session_\n"
    "  daemon showDB --json\n";

CmdResult handleHelp(const json&) {
    return CmdResult(0, HELP_MESSAGE);
}

CmdResult handleOpenedTty(const json& command) {
    return Terminal::openedTty(command);
}

CmdResult handleClosedTty(const json& command) {
    return Terminal::closedTty(command);
}

CmdResult handleUpdateDirHistory(const json& command) {
    return Terminal::updateDirHistory(command);
}

CmdResult handleCdForward(const json& command) {
    return Terminal::cdForward(command);
}

CmdResult handleCdBackward(const json& command) {
    return Terminal::cdBackward(command);
}

CmdResult handleShowTerminalInstance(const json& command) {
    return Terminal::showTerminalInstance(command);
}

CmdResult handleShowAllTerminalInstances(const json& command) {
    return Terminal::showAllTerminalInstances(command);
}

CmdResult handleShowEntriesByPrefix(const json& command) {
    string prefix = command[COMMAND_ARG_PREFIX].get<string>();
    auto entries = kvTable.getByPrefix(prefix);
    return CmdResult(0, formatEntriesAsText(entries));
}

CmdResult handleDeleteEntriesByPrefix(const json& command) {
    string prefix = command[COMMAND_ARG_PREFIX].get<string>();
    int rc = kvTable.deleteByPrefix(prefix);
    if (rc == SQLITE_OK) {
        return CmdResult(0, "Entries deleted\n");
    }
    return CmdResult(1, errorDeleteFailed(prefix));
}

CmdResult handleShowDb(const json&) {
    return CmdResult(0, formatEntriesAsText(kvTable.getAll()));
}

CmdResult handleDeleteEntry(const json& command) {
    string key = command[COMMAND_ARG_KEY].get<string>();
    int rc = kvTable.deleteEntry(key);
    if (rc == SQLITE_OK) {
        return CmdResult(0, "Entry deleted\n");
    }
    return CmdResult(1, errorEntryNotFound(key));
}

CmdResult handlePrintDirHistory(const json&) {
    std::stringstream ss;
    ss << "--- Directory History Entries ---\n";
    vector<std::pair<string, string>> dirs = kvTable.getByPrefix(DIR_HISTORY_ENTRY_PREFIX);
    std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    if (dirs.empty()) {
        ss << "No directory entries found.\n";
    } else {
        for (const auto& pair : dirs) {
            ss << "  " << pair.first << ": " << pair.second << "\n";
        }
    }

    ss << "\n--- TTY Pointers to History Entries ---\n";
    vector<std::pair<string, string>> ptsPointers = kvTable.getByPrefix(DIR_HISTORY_POINTER_PREFIX);
    std::sort(ptsPointers.begin(), ptsPointers.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    if (ptsPointers.empty()) {
        ss << "No TTY pointers found.\n";
    } else {
        for (const auto& pair : ptsPointers) {
            ss << "  " << pair.first << ": " << pair.second << "\n";
        }
    }
    
    ss << "\n--- Last Touched Directory ---\n";
    string lastTouchedIndex = kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
    if (!lastTouchedIndex.empty()) {
        string lastTouchedKey = string(DIR_HISTORY_ENTRY_PREFIX) + lastTouchedIndex;
        string lastTouchedValue = kvTable.get(lastTouchedKey);
        ss << "  Index: " << lastTouchedIndex << ", Path: " << lastTouchedValue << "\n";
    } else {
        ss << "No last touched directory recorded.\n";
    }
    
    return CmdResult(0, ss.str());
}

CmdResult handleUpsertEntry(const json& command) {
    string key = command[COMMAND_ARG_KEY].get<string>();
    string value = command[COMMAND_ARG_VALUE].get<string>();
    int rc = kvTable.upsert(key, value);
    if (rc == SQLITE_OK) {
        return CmdResult(0, "Entry upserted\n");
    }
    string errorMsg = string("Upsert failed with code ") + to_string(rc) + ": " + sqlite3_errstr(rc) + "\n";
    return CmdResult(rc, errorMsg);
}

CmdResult handleGetEntry(const json& command) {
    string key = command[COMMAND_ARG_KEY].get<string>();
    string value = kvTable.get(key);
    if (value.empty()) {
        return CmdResult(1, "\n");
    }
    return CmdResult(0, value + "\n");
}
CmdResult handlePing(const json&) {
    return CmdResult(0, "pong\n");
}
CmdResult handleGetKeyboardPath(const json&) {
    string path = kvTable.get("keyboardPath");
    if (path.empty()) {
        return CmdResult(1, "Keyboard path not found\n");
    }
    return CmdResult(0, path + "\n");
}

static string readScriptFile(const string& relativeScriptPath) {
    string scriptContent;
    std::ifstream scriptFile(relativeScriptPath);
    if (!scriptFile.is_open()) {
        string logMessage = string("[ERROR] Failed to open script file: ") + relativeScriptPath + "\n";
        logToFile(logMessage);
        return "";
    }
    std::stringstream buffer;
    buffer << scriptFile.rdbuf();
    scriptContent = buffer.str();
    scriptFile.close();
    return scriptContent;
}

static string substituteVariable(const string& content, const string& variable, const string& value) {
    string result = content;
    size_t pos = 0;
    string searchStr = string("$") + variable;
    while ((pos = result.find(searchStr, pos)) != string::npos) {
        result.replace(pos, searchStr.length(), value);
        pos += value.length();
    }
    return result;
}

CmdResult handleShouldLog(const json& command) {
    string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
    shouldLog = (enableStr == COMMAND_VALUE_TRUE);
    return CmdResult(0, string("Logging ") + (shouldLog ? "enabled" : "disabled") + "\n");
}

CmdResult handleToggleKeyboardsWhenActiveWindowChanges(const json& command) {
    string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
    toggleKeyboardsWhenActiveWindowChanges = (enableStr == COMMAND_VALUE_TRUE);
    return CmdResult(0, string("Return to default keyboard on next window change: ") + (toggleKeyboardsWhenActiveWindowChanges ? "no" : "yes") + "\n");
}

CmdResult handleGetDir(const json& command) {
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

CmdResult handleGetFile(const json& command) {
    string fileName = command[COMMAND_ARG_FILE_NAME].get<string>();
    for (const auto& file : files.files) {
        if (file.name.find(fileName) != string::npos) {
            return CmdResult(0, file.fullPath() + "\n");
        }
    }
    return CmdResult(1, "File not found: " + fileName + "\n");
}

CmdResult handleQuit(const json&) {
    running = 0; // Signal the daemon to shut down
    return CmdResult(0, "Shutting down daemon.\n");
}

CmdResult handleSetKeyboard(const json& command) {
    static string previousKeyboard = "";
    string keyboardName = command[COMMAND_ARG_KEYBOARD_NAME].get<string>();
    if (keyboardName == previousKeyboard) {
        return CmdResult(0, "Keyboard already set to: " + keyboardName + "\n");
    }
    // else {
    //     return CmdResult(0, "Keyboard in test mode: " + keyboardName + "\n");
    // }
    previousKeyboard = keyboardName;
    string logMessage;
    bool isKnown = false;
    FILE* pipe;
    string output;
    int status;
    int exitCode;
    for (const string& known : KNOWN_KEYBOARDS) {
        if (known == keyboardName) {
            isKnown = true;
            break;
        }
    }
    if (!isKnown) {
        keyboardName = DEFAULT_KEYBOARD;
    }
    logMessage = string("[START setKeyboard] keyboard: ") + keyboardName + " isKnown: " + (isKnown ? "true" : "false") + "\n";
    logToFile(logMessage);
    string scriptPath = directories.mappings + PREFIX_KEYBOARD + keyboardName + ".sh";
    string scriptContent = readScriptFile(scriptPath);
    if (scriptContent.empty()) {
        return CmdResult(1, "Script file not found\n");
    }
    scriptContent = substituteVariable(scriptContent, KEYBOARD_PATH_KEY, kvTable.get(KEYBOARD_PATH_KEY));
    scriptContent = substituteVariable(scriptContent, MOUSE_PATH_KEY, kvTable.get(MOUSE_PATH_KEY));
    scriptContent = substituteVariable(scriptContent, EVSIEVE_RANDOM_VAR, to_string(rand() % 1000000));
    string cmd = string(
        "sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; "
        "sudo systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service "
        "--property=StandardError=append:" + directories.data + EVSIEVE_STANDARD_ERR_FILE + " "
        "--property=StandardOutput=append:" + directories.data + EVSIEVE_STANDARD_OUTPUT_FILE + " "
        )
        + scriptContent
    ;
    if (! toggleKeyboardsWhenActiveWindowChanges){
        cmd = string(
        "sudo systemctl stop corsairKeyBoardLogiMouse 2>&1 ; ");
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
    logMessage = string("[STATUS] raw status: ") + to_string(status) + " exit code: " + to_string(exitCode) + "\n";
    logToFile(logMessage);
    if (status != 0) {
        logMessage = string("[END] FAILED\n");
        logToFile(logMessage);
        return CmdResult(1, string("Failed to execute (exit code ") + std::to_string(exitCode) + ", output: " + output + ")\n");
    }
    logMessage = string("[END] SUCCESS\n");
    logToFile(logMessage);
    return CmdResult(0, string("SUCCESS\n" + output + "Set keyboard to: ") + keyboardName + "\n");
}

typedef CmdResult (*CommandHandler)(const json&);

struct CommandDispatch {
    const char* name;
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
    {COMMAND_SET_KEYBOARD, handleSetKeyboard},
    {COMMAND_SHOULD_LOG, handleShouldLog},
    {COMMAND_TOGGLE_KEYBOARDS_WHEN_ACTIVE_WINDOW_CHANGES, handleToggleKeyboardsWhenActiveWindowChanges},
    {COMMAND_GET_DIR, handleGetDir},
    {COMMAND_GET_FILE, handleGetFile},
    {COMMAND_QUIT, handleQuit},
};

static const size_t COMMAND_HANDLERS_SIZE = sizeof(COMMAND_HANDLERS) / sizeof(COMMAND_HANDLERS[0]);

CmdResult testIntegrity(const json& command) {
    if (!command.contains(COMMAND_KEY)) {
        return CmdResult(1, "Missing command key");
    }
    string commandName = command[COMMAND_KEY].get<string>();
    const CommandSignature* foundCommand = nullptr;
    for (size_t i = 0; i < COMMAND_REGISTRY_SIZE; ++i) {
        if (COMMAND_REGISTRY[i].name == commandName) {
            foundCommand = &COMMAND_REGISTRY[i];
            break;
        }
    }
    if (!foundCommand) {
        return CmdResult(1, string("Unknown command: ") + commandName + mustEndWithNewLine);
    }
    for (const string& arg : foundCommand->requiredArgs) {
        if (!command.contains(arg)) {
            return CmdResult(1, string("Missing required arg: ") + arg + mustEndWithNewLine);
        }
    }
    return CmdResult(0, "");
}

int mainCommand(const json& command, int client_sock) {
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
                result.message = string("Unhandled command: ") + commandStr + mustEndWithNewLine;
            }
        }
    } catch (const std::exception& e) {
        result.status = 1;
        result.message = std::string("error: ") + e.what() + "\n";
    }
    if (!result.message.empty() && result.message.back() != '\n') {
        result.message += "daemon must end in \\n\n";
    }
    if (command[COMMAND_KEY] == "closedTty") {
        return 0;
    }
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}