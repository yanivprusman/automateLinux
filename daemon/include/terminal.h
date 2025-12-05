#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "KVTable.h"

class Terminal {
    public:
        Terminal( int tty );
        ~Terminal();
        static set<Terminal*> instances;
        static CmdResult openedTty(const json& command);
        CmdResult _openedTty(const json& command);
        int tty;
        string dirHistoryEntryKey(int index);
        string dirHistoryPointerKey(int index);
};

#endif // TERMINAL_H
