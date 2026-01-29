#ifndef CMD_APP_H
#define CMD_APP_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// App Assignment Command Handlers
CmdResult handleClaimApp(const json &command);
CmdResult handleReleaseApp(const json &command);
CmdResult handleListApps(const json &command);
CmdResult handleGetAppOwner(const json &command);

// VPS-specific Commands
CmdResult handleUpdateNginxForward(const json &command);

#endif // CMD_APP_H
