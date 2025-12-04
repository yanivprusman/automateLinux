#include "terminal.h"

set<Terminal*> Terminal::instances;

Terminal::Terminal() {
    instances.insert(this);
}

Terminal::~Terminal() {
    instances.erase(this);
}

CmdResult Terminal::openedTty(const json& command) {
    Terminal* terminal = new Terminal();
    return terminal->_openedTty(command);
}

CmdResult Terminal::_openedTty(const json& command) {
    CmdResult result;
    tty = command[TTY_KEY].get<int>();
    string indexString = kvTable.get(IDNEX_OF_LAST_TOUCHED_DIR_KEY);
    if (indexString.empty()) {
        kvTable.upsert(IDNEX_OF_LAST_TOUCHED_DIR_KEY, "0");
        kvTable.upsert(dirHistoryKey(0), DIR_HISTORY_DEFAULT_DIR);
        indexString = kvTable.get(IDNEX_OF_LAST_TOUCHED_DIR_KEY);
        // result.message = DIR_HISTORY_DEFAULT_DIR;
        // return result;
    }
    int index = atoi (indexString.c_str());
    result.message = kvTable.get(dirHistoryKey(index)) + mustEndWithNewLine;
    result.status = 0;
    // result.message = "myDir\n";
    return result;
}

string Terminal::dirHistoryKey(int index) {
    return DIR_HISTORY_POINTER_PREFIX + to_string(index);
}
