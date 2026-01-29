#ifndef CMD_DATABASE_H
#define CMD_DATABASE_H

#include "Types.h"
#include <json/json.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Database/Settings Command Handlers
CmdResult handleShowDb(const json &command);
CmdResult handleUpsertEntry(const json &command);
CmdResult handleGetEntry(const json &command);
CmdResult handleDeleteEntry(const json &command);
CmdResult handleShowEntriesByPrefix(const json &command);
CmdResult handleDeleteEntriesByPrefix(const json &command);

#endif // CMD_DATABASE_H
