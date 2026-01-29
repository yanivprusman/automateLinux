#ifndef CMD_TERMINAL_H
#define CMD_TERMINAL_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Terminal/TTY Command Handlers
CmdResult handleOpenedTty(const json &command);
CmdResult handleClosedTty(const json &command);
CmdResult handleUpdateDirHistory(const json &command);
CmdResult handleCdForward(const json &command);
CmdResult handleCdBackward(const json &command);
CmdResult handleShowTerminalInstance(const json &command);
CmdResult handleShowAllTerminalInstances(const json &command);
CmdResult handlePrintDirHistory(const json &command);
CmdResult handleEmptyDirHistoryTable(const json &command);
CmdResult handleShellSignal(const json &command);

#endif // CMD_TERMINAL_H
