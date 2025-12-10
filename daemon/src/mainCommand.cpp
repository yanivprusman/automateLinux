#include "mainCommand.h"

static int g_client_sock = -1;

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
    "  ping                    Ping the daemon and receive pong response.\n\n"
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
    string result = "directories:\n";
    vector<std::pair<string, string>> dirs = kvTable.getByPrefix(DIR_HISTORY_ENTRY_PREFIX);
    std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    result += formatEntriesAsText(dirs);
    vector<std::pair<string, string>> ptsPointers = kvTable.getByPrefix(DIR_HISTORY_POINTER_PREFIX);
    std::sort(ptsPointers.begin(), ptsPointers.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    result += "Pointers:\n";
    result += formatEntriesAsText(ptsPointers);
    string lastTouched = string(DIR_HISTORY_ENTRY_PREFIX) + kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
    result += string("last touched ") + lastTouched + " " + kvTable.get(lastTouched) + mustEndWithNewLine;
    return CmdResult(0, result);
}

CmdResult handleUpsertEntry(const json& command) {
    string key = command[COMMAND_ARG_KEY].get<string>();
    string value = command[COMMAND_ARG_VALUE].get<string>();
    kvTable.upsert(key, value);
    return CmdResult(0, "Entry upserted\n");
}

CmdResult handleGetEntry(const json& command) {
    string key = command[COMMAND_ARG_KEY].get<string>();
    string value = kvTable.get(key);
    if (value.empty()) {
        return CmdResult(1, "");
    }
    return CmdResult(0, value + "\n");
}
CmdResult handlePing(const json&) {
    return CmdResult(0, "pong\n");
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
    g_client_sock = client_sock;
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
    if (isMultiline(result.message)) {
        result.message = toJsonSingleLine(result.message);
        result.message += "\n";
    }
    if (command[COMMAND_KEY] == "closedTty") {
        return 0;
    }
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}