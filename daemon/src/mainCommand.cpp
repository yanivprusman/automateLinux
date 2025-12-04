#include "mainCommand.h"

CmdResult openedTty(const json& command){
    // kvTable.countKeysByPrefix
    return Terminal::openedTty(command);
}

int mainCommand(const json& command, int client_sock, ucred cred) {   
    CmdResult result;   
    try {
        if (command[COMMAND_KEY] == COMMAND_OPENED_TTY) {
            result = openedTty(command);
        } else {
            result.status = 1;
            result.message = "unknown command " + command[COMMAND_KEY].get<string>() + "\n";
        }   
    } catch (const std::exception& e) {
        result.status = 1;
        result.message = std::string("error: ") + e.what() + "\n";
    }
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}

// if (command == "ttyOpened") {
//     result= "Known command " + System::getTty() + "\n";
    
// } else {
//     result = "ERROR: Unknown command\n";
// }



// if (dirHistory.numberOfPointers() == 0) {
//     dirHistory.resetToBeginningState();

// result= "Known command " + string(ttyname(client_sock)) + "\n";
// result= "Known command " + to_string(cred.pid) + "\n";
