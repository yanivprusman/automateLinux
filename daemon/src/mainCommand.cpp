#include "mainCommand.h"

int mainCommand(const json& command, int client_sock, ucred cred) {   
    (void)cred;  // unused
    string commandStr = command.dump();
    CmdResult result;   
    try {
        if (command[COMMAND_KEY] == COMMAND_OPENED_TTY) {
            result = Terminal::openedTty(command);
        }else if (command[COMMAND_KEY] == COMMAND_CLOSED_TTY) {
            result = Terminal::closedTty(command);
        } else if (command[COMMAND_KEY] == COMMAND_UPDATE_DIR_HISTORY) {
            result = Terminal::updateDirHistory(command);
        } else if (command[COMMAND_KEY] ==  COMMAND_CDFORWARD) {
            result = Terminal::cdForward(command);
        } else if (command[COMMAND_KEY] == COMMAND_CDBACKWARD) {
            result = Terminal::cdBackward(command);
        } else if (command[COMMAND_KEY] == COMMAND_SHOW_INDEX) {
            result = Terminal::showIndex(command);
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ALL_DIR_ENTRIES) {
            result = Terminal::deleteAllDirEntries(command);
        } else if (command[COMMAND_KEY] == COMMAND_LIST_ALL_ENTRIES) {
            result = Terminal::listAllEntries(command);
        } else if (command[COMMAND_KEY] == COMMAND_DELETE_ENTRY) {
            if (command.contains("key")) {
                int keyExists = kvTable.deleteEntry(command["key"].get<string>());
                if (keyExists == SQLITE_OK) {
                result.status = 0;
                result.message = "Entry deleted\n";
                }
                else {
                    result.status = 1;
                    result.message = "Entry not found for key " + command["key"].get<string>() + "\n";
                }
            } else {
                result.status = 1;
                result.message = "missing key for deleteEntry command\n";
            }
        } else {
            result.status = 1;
            result.message = "unknown command " + command[COMMAND_KEY].get<string>() + "\n";
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
    if (command[COMMAND_KEY] == COMMAND_CLOSED_TTY) {
        return 0;
    }
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}

