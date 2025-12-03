#include "mainCommand.h"

int openedTty(const json& command){
    // kvTable.countKeysByPrefix
    // dirHistory
    Terminal::openedTty(command);
    return 0;
}

int mainCommand(const json& command, int client_sock, ucred cred) {   
    string result; 
    if (command[COMMAND_KEY] == COMMAND_OPENED_TTY) {
        result = openedTty(command);
    } else {
        result= "unknown command " + command[COMMAND_KEY].get<string>() + "\n";
    }   
    // result = "return anything until implemented\n";
    write(client_sock, result.c_str(), result.length());
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
