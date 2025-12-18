#ifndef MAINCOMMAND_H
#define MAINCOMMAND_H
#include <unordered_map>
#include "system.h"
#include "common.h"
#include "KVTable.h"
#include "dirHistory.h"
#include "terminal.h"

CmdResult testIntegrity(const json& command);
CmdResult handleActiveWindowChanged(const json& command);
int mainCommand(const json& command, int client_sock);
#endif // MAINCOMMAND_H


