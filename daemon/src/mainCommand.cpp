#include "mainCommand.h"

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
    // asdf
    string commandStr = command.dump();
    CmdResult result;   
    try {
        CmdResult integrityCheck = testIntegrity(command);
        if (integrityCheck.status != 0) {
            result = integrityCheck;
        } else if (command[COMMAND_KEY] == COMMAND_EMPTY ||
                   command[COMMAND_KEY] == COMMAND_HELP_DDASH ){
            result.status = 0;
            result.message =
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
                "  showDB                  Display all entries in the database.\n\n"
                "Options:\n"
                "  --help                  Display this help message.\n"
                "  --json                  Output results in JSON format.\n\n"
                "Examples:\n"
                "  daemon openedTty\n"
                "  daemon cdForward\n"
                "  daemon deleteEntriesByPrefix session_\n"
                "  daemon showDB --json\n";
        } else if (command[COMMAND_KEY] == COMMAND_OPENED_TTY) {
            result = Terminal::openedTty(command);
        } else if (command[COMMAND_KEY] == COMMAND_CLOSED_TTY) {
            result = Terminal::closedTty(command);
        } else if (command[COMMAND_KEY] == COMMAND_UPDATE_DIR_HISTORY) {
            result = Terminal::updateDirHistory(command);
        } else if (command[COMMAND_KEY] == COMMAND_CD_FORWARD) {
            result = Terminal::cdForward(command);
        } else if (command[COMMAND_KEY] == COMMAND_CD_BACKWARD) {
            result = Terminal::cdBackward(command);
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_TERMINAL_INSTANCE) {
            result = Terminal::showTerminalInstance(command);
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_ALL_TERMINAL_INSTANCES) {
            result = Terminal::showAllTerminalInstances(command);
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_ENTRIES_BY_PREFIX) {
            if (command.contains(COMMAND_ARG_PREFIX)) {
                string prefix = command[COMMAND_ARG_PREFIX].get<string>();
                auto entries = kvTable.getByPrefix(prefix);
                result.message = formatEntriesAsText(entries);
                result.status = 0;
            } else {
                result.status = 1;
                result.message = "Missing required arg: prefix\n";
            }
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ENTRIES_BY_PREFIX) {
            string prefix = command[COMMAND_ARG_PREFIX].get<string>();
            int rc = kvTable.deleteByPrefix(prefix);
            if (rc == SQLITE_OK) {
                result.status = 0;
                result.message = "Entries deleted\n";
            } else {
                result.status = 1;
                result.message = "Error deleting entries with prefix: " + prefix + "\n";
            }
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_DB) {
            result.message = formatEntriesAsText(kvTable.getAll());
            result.status = 0;
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ENTRY) {
            if (command.contains(COMMAND_ARG_KEY)) {
                int keyExists = kvTable.deleteEntry(command[COMMAND_ARG_KEY].get<string>());
                if (keyExists == SQLITE_OK) {
                    result.status = 0;
                    result.message = "Entry deleted\n";
                } else {
                    result.status = 1;
                    result.message = "Entry not found for key " + command[COMMAND_ARG_KEY].get<string>() + "\n";
                }
            } else {
                result.status = 1;
                result.message = "Missing required arg: key\n";
            }
        } else if (command[COMMAND_KEY] == COMMAND_PRINT_DIR_HISTORY) {
            result.message = "directories:\n";
            vector<std::pair<string, string>> dirs = kvTable.getByPrefix(DIR_HISTORY_ENTRY_PREFIX);
            std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
            result.message += formatEntriesAsText(dirs);
            vector<std::pair<string, string>> ptsPointers = kvTable.getByPrefix(DIR_HISTORY_POINTER_PREFIX);
            std::sort(ptsPointers.begin(), ptsPointers.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
            result.message += "Pointers:\n";
            result.message += formatEntriesAsText(ptsPointers);
            string lastTouched = string(DIR_HISTORY_ENTRY_PREFIX) + kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
            result.message += string("last touched ") +   lastTouched + " " + kvTable.get(lastTouched) + mustEndWithNewLine; 
            result.status = 0;
        } else if (command[COMMAND_KEY] == COMMAND_UPSERT_ENTRY) {
            if (command.contains(COMMAND_ARG_KEY) && command.contains(COMMAND_ARG_VALUE)) {
                string key = command[COMMAND_ARG_KEY].get<string>();
                string value = command[COMMAND_ARG_VALUE].get<string>();
                kvTable.upsert(key, value);
                result.status = 0;
                result.message = "Entry upserted\n";
            } else {
                result.status = 1;
                result.message = "Missing required args: key, value\n";
            }
        } else if (command[COMMAND_KEY] == COMMAND_GET_ENTRY) {
            if (command.contains(COMMAND_ARG_KEY)) {
                string key = command[COMMAND_ARG_KEY].get<string>();
                string value = kvTable.get(key);
                result.status = value.empty() ? 1 : 0;
                result.message = value.empty() ? "" : value + "\n";
            } else {
                result.status = 1;
                result.message = "Missing required arg: key\n";
            }
        }
        else {
            result.status = 1;
            result.message = string("Unhandled command: ") + commandStr + mustEndWithNewLine;
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

