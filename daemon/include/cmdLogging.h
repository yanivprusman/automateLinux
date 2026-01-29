#ifndef CMD_LOGGING_H
#define CMD_LOGGING_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Logging Command Handlers
CmdResult handleShouldLog(const json &command);
CmdResult handleGetShouldLog(const json &command);
CmdResult handleRegisterLogListener(const json &command);
CmdResult handleGetEventFilters(const json &command);
CmdResult handleSetEventFilters(const json &command);

// Provides access to the client socket for log listener registration
void setLoggingClientSocket(int socket);

#endif // CMD_LOGGING_H
