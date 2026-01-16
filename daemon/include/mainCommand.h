#ifndef MAINCOMMAND_H
#define MAINCOMMAND_H
#include "common.h"
#include "system.h"
#include "terminal.h"
#include <unordered_map>

CmdResult testIntegrity(const json &command);
CmdResult handleActiveWindowChanged(const json &command);
CmdResult handleIsLoomActive(const json &command);
CmdResult handleRestartLoom(const json &command);
int mainCommand(const json &command, int client_sock);
bool isNativeHostConnected();
#endif // MAINCOMMAND_H
