#include "terminal.h"

set<Terminal*> Terminal::instances;

Terminal::Terminal() {
    instances.insert(this);
}

Terminal::~Terminal() {
    instances.erase(this);
}

int Terminal::openedTty(const json& command) {
    Terminal* terminal = new Terminal();
    terminal->_openedTty(command);
    return 0;
}

string Terminal::_openedTty(const json& command) {
    tty = command[TTY_KEY].get<int>();   
    string value = kvTable.get(IDNEX_OF_LAST_TOUCHED_DIR_KEY);
    if (value.empty()) {
        kvTable.upsert(IDNEX_OF_LAST_TOUCHED_DIR_KEY, "0");
        kvTable.upsert(dirHistoryKey(0), DIR_HISTORY_DEFAULT_DIR);
        return DIR_HISTORY_DEFAULT_DIR;
    }
    int index = atoi (value.c_str());

    return "myDir";
}

string Terminal::dirHistoryKey(int index) {
    return DIR_HISTORY_POINTER_PREFIX + to_string(index);
}
