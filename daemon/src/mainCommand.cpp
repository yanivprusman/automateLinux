#include "mainCommand.h"

CmdResult openedTty(const json& command){
    return Terminal::openedTty(command);
}

int mainCommand(const json& command, int client_sock, ucred cred) {   
    (void)cred;  // unused
    CmdResult result;   
    try {
        if (command[COMMAND_KEY] == COMMAND_OPENED_TTY) {
            result = openedTty(command);
        } else if (command[COMMAND_KEY] == COMMAND_UPDATE_DIR_HISTORY) {
            result = Terminal::updateDirHistory(command);
        } else if (command[COMMAND_KEY] == "cdForward") {
            result = Terminal::cdForward(command);
        } else if (command[COMMAND_KEY] == "cdBackward") {
            result = Terminal::cdBackward(command);
        } else if (command[COMMAND_KEY] == "showIndex") {
            result = Terminal::showIndex(command);
        } else if (command[COMMAND_KEY] == "deleteAllDirEntries") {
            result = Terminal::deleteAllDirEntries(command);
        } else if (command[COMMAND_KEY] == "listAllEntries") {
            result = Terminal::listAllEntries(command);
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
        result.message = toJsonSingleLine(result.message) + "\n";
    }    
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}

