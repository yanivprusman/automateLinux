#ifndef CMD_BROWSER_H
#define CMD_BROWSER_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Chrome Extension/Browser Command Handlers
CmdResult handleSetActiveTabUrl(const json &command);
CmdResult handleRegisterNativeHost(const json &command);
CmdResult handleFocusChatGPT(const json &command);
CmdResult handleFocusAck(const json &command);

// Utility functions for browser integration
std::string getActiveTabUrlFromExtension();
void triggerChromeChatGPTFocus();
bool isNativeHostConnected();

// Provides access to the client socket for native host registration
void setBrowserClientSocket(int socket);

#endif // CMD_BROWSER_H
