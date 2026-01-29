#ifndef CMD_SYSTEM_H
#define CMD_SYSTEM_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// System Command Handlers
CmdResult handleHelp(const json &command);
CmdResult handlePing(const json &command);
CmdResult handleQuit(const json &command);
CmdResult handleGetDir(const json &command);
CmdResult handleGetFile(const json &command);
CmdResult handleGetSocketPath(const json &command);
CmdResult handleListCommands(const json &command);

// Test/Debug Commands
CmdResult handleTestLsof(const json &command);
CmdResult handleTestEcho(const json &command);
CmdResult handleTestLsofScript(const json &command);

#endif // CMD_SYSTEM_H
