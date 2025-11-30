#ifndef MAINCOMMAND_H
#define MAINCOMMAND_H
#include "common.h"
#include "KVTable.h"
#include "dirHistory.h"
int mainCommand(const json& command, int client_sock, ucred cred);
#endif // MAINCOMMAND_H


