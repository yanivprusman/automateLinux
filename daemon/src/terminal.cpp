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
    dirHistoryPointerKey = DIR_HISTORY_POINTER_PREFIX + to_string(tty);
    kvTable.upsert(dirHistoryPointerKey, indexString);
}

Terminal::~Terminal() {
    instances.erase(this);
}

CmdResult Terminal::openedTty(const json& command) {
    Terminal* terminal = new Terminal(command[TTY_KEY].get<int>());
    return terminal->_openedTty(command);
}

CmdResult Terminal::_openedTty(const json& command) {
    (void)command;  // unused
    CmdResult result;
    int index = stoi(kvTable.get(dirHistoryPointerKey));
    result.message = kvTable.get(dirHistoryEntryKey(index)) + mustEndWithNewLine;
    result.status = 0;
    return result;
}

string Terminal::dirHistoryEntryKey(int index) {
    return DIR_HISTORY_POINTER_PREFIX + to_string(index);
}

CmdResult Terminal::updateDirHistory(const json& command) {
    for (Terminal* terminal : instances) {
        if (terminal->tty == command[TTY_KEY].get<int>()) {
            return terminal->_updateDirHistory(command);
        }
    }
    CmdResult result;
    result.status = 1;
    result.message = "Terminal instance not found for tty " + to_string(command[TTY_KEY].get<int>()) + "\n";
    return result;  
}

int Terminal::getIndex() {
    return stoi(kvTable.get(dirHistoryPointerKey));
}

string Terminal::getPWD(const json& command) {
    return command[PWD_KEY].get<string>();
}

string Terminal::getDirHistoryEntry(int index) {
    return kvTable.get(dirHistoryEntryKey(index));
}

string Terminal::dirHistoryKeyPrefix() {
    return DIR_HISTORY_POINTER_PREFIX;
}

CmdResult Terminal::_updateDirHistory(const json& command) {
    CmdResult result;
    int index = getIndex();
    string pwd = getPWD(command);
    string currentDir = getDirHistoryEntry(index);
    string nextDir = getDirHistoryEntry(index+1);
    if (pwd.empty()) {
        result.status = 1;
        result.message = "No directory provided to updateDirHistory\n";
        return result;
    }else if (currentDir == pwd) {
        result.status = 0;
        result.message = "Directory is the same as the last one, not updating\n";
        return result;
    }else if (nextDir == pwd) {
        kvTable.upsert(dirHistoryPointerKey, to_string(index+1));
        result.status = 0;
        result.message = "Moved forward in directory history\n";
        return result;
    }else{
        int insertIndex = index + 1;
        kvTable.insertAt(dirHistoryKeyPrefix(), insertIndex, pwd);
        kvTable.upsert(dirHistoryPointerKey, to_string(insertIndex));
        result.status = 0;
        result.message = "New directory inserted in history\n";
        return result;
    }
}

CmdResult Terminal::cdForward(const json& command) {
    for (Terminal* terminal : instances) {
        if (terminal->tty == command[TTY_KEY].get<int>()) {
            return terminal->_cdForward(command);
        }
    }
    CmdResult result;
    result.status = 1;
    result.message = "Terminal instance not found for tty " + to_string(command[TTY_KEY].get<int>()) + "\n";
    return result;
}

CmdResult Terminal::_cdForward(const json& command) {
    (void)command;  // unused
    CmdResult result;
    int index = getIndex();
    string nextDir = getDirHistoryEntry(index + 1);
    
    if (nextDir.empty()) {
        result.status = 1;
        result.message = "Cannot move forward, no next directory in history\n";
        return result;
    }
    
    kvTable.upsert(dirHistoryPointerKey, to_string(index + 1));
    result.message = nextDir + mustEndWithNewLine;
    result.status = 0;
    return result;
}

CmdResult Terminal::cdBackward(const json& command) {
    for (Terminal* terminal : instances) {
        if (terminal->tty == command[TTY_KEY].get<int>()) {
            return terminal->_cdBackward(command);
        }
    }
    CmdResult result;
    result.status = 1;
    result.message = "Terminal instance not found for tty " + to_string(command[TTY_KEY].get<int>()) + "\n";
    return result;
}

CmdResult Terminal::_cdBackward(const json& command) {
    (void)command;  // unused
    CmdResult result;
    int index = getIndex();
    
    if (index == 0) {
        result.status = 1;
        result.message = "Cannot move backward, already at beginning of history\n";
        return result;
    }
    
    string prevDir = getDirHistoryEntry(index - 1);
    kvTable.upsert(dirHistoryPointerKey, to_string(index - 1));
    result.message = prevDir + mustEndWithNewLine;
    result.status = 0;
    return result;
}
