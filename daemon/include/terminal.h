#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "KVTable.h"

class Terminal {
    public:
        Terminal( int tty );
        ~Terminal();
        static vector<Terminal*> instances;
        static Terminal* getInstanceByTTY(int tty);
        static string dirHistoryEntryKey(int index);
        static CmdResult openedTty(const json& command);
        CmdResult _openedTty(const json& command);
        static CmdResult closedTty(const json& command);
        CmdResult _closedTty(const json& command);
        static CmdResult updateDirHistory(const json& command);
        CmdResult _updateDirHistory(const json& command);
        static CmdResult cdForward(const json& command);
        CmdResult _cdForward(const json& command);
        static CmdResult cdBackward(const json& command);
        CmdResult _cdBackward(const json& command);
        static CmdResult showTerminalInstance(const json& command);
        static CmdResult showAllTerminalInstances(const json& command);
        int tty;
        string dirHistoryPointerKey;
        int getIndex();
        static string getPWD(const json& command);
        static string getDirHistoryEntry(int index);
        static string dirHistoryKeyPrefix();
        string toString();
};

#endif // TERMINAL_H
