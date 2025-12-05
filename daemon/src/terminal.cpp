#include "terminal.h"

set<Terminal*> Terminal::instances;

Terminal::Terminal(int tty) : tty(tty) {
    instances.insert(this);
    string indexString = kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
    if (indexString.empty()) {
        kvTable.upsert(dirHistoryEntryKey(0), DIR_HISTORY_DEFAULT_DIR);
        kvTable.upsert(INDEX_OF_LAST_TOUCHED_DIR_KEY, "0");
        indexString = kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
    }
    kvTable.upsert(dirHistoryPointerKey(tty), indexString);
}

Terminal::~Terminal() {
    instances.erase(this);
}

CmdResult Terminal::openedTty(const json& command) {
    Terminal* terminal = new Terminal(command[TTY_KEY].get<int>());
    return terminal->_openedTty(command);
}

CmdResult Terminal::_openedTty(const json& command) {
    CmdResult result;
    int index = atoi (kvTable.get(dirHistoryPointerKey(tty)).c_str());
    result.message = kvTable.get(dirHistoryEntryKey(index)) + mustEndWithNewLine;
    result.status = 0;
    return result;
}

string Terminal::dirHistoryEntryKey(int index) {
    return DIR_HISTORY_POINTER_PREFIX + to_string(index);
}

string Terminal::dirHistoryPointerKey(int index) {
    return DIR_HISTORY_PREFIX + to_string(index);
}