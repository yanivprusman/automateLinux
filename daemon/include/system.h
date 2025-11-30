#ifndef SYSTEM_H
#define SYSTEM_H

#include "common.h"

class System {
public:
    // Get current TTY
    static string getTty();
    
    // Execute bash command and return output
    static string executeBashCommand(const char* cmd);
};

using ExecuteBashCommandFn = string(*)(const char*);
static constexpr ExecuteBashCommandFn executeBashCommand = &System::executeBashCommand;

#endif // SYSTEM_H
