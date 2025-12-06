#include "mainCommand.h"

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
        return CmdResult(1, "Unknown command: " + commandName + mustEndWithNewLine);
    }
    for (const string& arg : foundCommand->requiredArgs) {
        if (!command.contains(arg)) {
            return CmdResult(1, "Missing required arg: " + arg + mustEndWithNewLine);
        }
    }
    return CmdResult(0, "");
}

int mainCommand(const json& command, int client_sock) {   
    string commandStr = command.dump();
    CmdResult result;   
    try {
        CmdResult integrityCheck = testIntegrity(command);
        if (integrityCheck.status != 0) {
            result = integrityCheck;
        } else if (command[COMMAND_KEY] == COMMAND_EMPTY ||
                   command[COMMAND_KEY] == COMMAND_HELP ||
                   command[COMMAND_KEY] == COMMAND_HELP_DASH ||
                   command[COMMAND_KEY] == COMMAND_HELP_DDASH ||
                   command[COMMAND_KEY] == COMMAND_HELP_H) {
            result.status = 0;
            result.message =
                "Available commands:\n"
                "1. openedTty - Notify daemon that a terminal has been opened.\n"
                "2. closedTty - Notify daemon that a terminal has been closed.\n"
                "3. updateDirHistory - Update the directory history for a terminal.\n"
                "4. cdForward - Move forward in the directory history for a terminal.\n"
                "5. cdBackward - Move backward in the directory history for a terminal.\n"
                "6. showIndex - Show the current index in the directory history for a terminal.\n"
                "7. deleteEntry - Delete a specific entry from the database by key.\n"
                "8. deleteEntriesByPrefix - Delete all entries from the database with a specific prefix.\n"
                "9. showDB - Display all entries in the database.\n";
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
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_INDEX) {
            result = Terminal::showIndex(command);
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ENTRIES_BY_PREFIX) {
            result = KVTable::deleteByPrefix(command);
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_DB) {
            auto allEntries = kvTable.getAll();
            result.status = 0;
            if (allEntries.empty()) {
                result.message = "Database is empty\n";
            } else {
                for (const auto& pair : allEntries) {
                    result.message += pair.first + "|" + pair.second + "\n";
                }
            }
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ENTRY) {
            int keyExists = kvTable.deleteEntry(command[COMMAND_ARG_KEY].get<string>());
            if (keyExists == SQLITE_OK) {
                result.status = 0;
                result.message = "Entry deleted\n";
            }else {
                result.status = 1;
                result.message = "Entry not found for key " + command[COMMAND_ARG_KEY].get<string>() + "\n";
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

