#ifndef CMD_LOOM_H
#define CMD_LOOM_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Loom Command Handlers
CmdResult handleIsLoomActive(const json &command);
CmdResult handleStopLoom(const json &command);
CmdResult handleRestartLoom(const json &command);
CmdResult handleGenerateLoomToken(const json &command);
CmdResult handleRevokeLoomTokens(const json &command);
CmdResult handleResetClock(const json &command);

#endif // CMD_LOOM_H
