#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"
#include "KVTable.h"

class Terminal {
    public:
        Terminal();
        ~Terminal();
        static set<Terminal*> instances;
        static int openedTty(const json& command);
        string _openedTty(const json& command);
        int tty;
        string dirHistoryKey(int index);
};

#endif // TERMINAL_H
