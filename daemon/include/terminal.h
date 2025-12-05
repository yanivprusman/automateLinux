#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "KVTable.h"

class Terminal {
    public:
        Terminal( int tty );
        ~Terminal();
        static set<Terminal*> instances;
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
        static CmdResult showIndex(const json& command);
        static CmdResult deleteAllDirEntries(const json& command);
        static CmdResult listAllEntries(const json& command);
        int tty;
        string dirHistoryPointerKey;
        int getIndex();
        static string getPWD(const json& command);
        static string getDirHistoryEntry(int index);
        static string dirHistoryKeyPrefix();
};

#endif // TERMINAL_H
