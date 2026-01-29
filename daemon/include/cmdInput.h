#ifndef CMD_INPUT_H
#define CMD_INPUT_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Input/Keyboard Command Handlers
CmdResult handleGetKeyboardPath(const json &command);
CmdResult handleGetMousePath(const json &command);
CmdResult handleSetKeyboard(const json &command);
CmdResult handleGetKeyboard(const json &command);
CmdResult handleToggleKeyboard(const json &command);
CmdResult handleDisableKeyboard(const json &command);
CmdResult handleEnableKeyboard(const json &command);
CmdResult handleSimulateInput(const json &command);
CmdResult handleTestIntegrity(const json &command);

// Macro-related handlers
CmdResult handleGetMacros(const json &command);
CmdResult handleUpdateMacros(const json &command);

// Helper function
void typeChar(char c);

#endif // CMD_INPUT_H
