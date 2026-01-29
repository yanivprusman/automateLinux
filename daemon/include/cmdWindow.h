#ifndef CMD_WINDOW_H
#define CMD_WINDOW_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Window/Context Command Handlers
CmdResult handleRegisterWindowExtension(const json &command);
CmdResult handleListWindows(const json &command);
CmdResult handleActivateWindow(const json &command);
CmdResult handleActiveWindowChanged(const json &command);
CmdResult handleGetActiveContext(const json &command);

// Provides access to the window extension socket for registration
void setWindowExtensionClientSocket(int socket);

#endif // CMD_WINDOW_H
