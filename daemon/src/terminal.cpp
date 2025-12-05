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
    try {
        string indexStr = kvTable.get(dirHistoryPointerKey);
        if (indexStr.empty()) {
            result.status = 1;
            result.message = "No directory history found\n";
            return result;
        }
        int index = stoi(indexStr);
        string dir = kvTable.get(dirHistoryEntryKey(index));
        if (dir.empty()) {
            result.status = 1;
            result.message = "Directory not found at index " + to_string(index) + "\n";
            return result;
        }
        result.message = dir + mustEndWithNewLine;
        result.status = 0;
    } catch (const std::exception& e) {
        result.status = 1;
        result.message = "Error in openedTty: " + std::string(e.what()) + "\n";
    }
    return result;
}

CmdResult Terminal::closedTty(const json& command) {
    int tty = command[TTY_KEY].get<int>();
    for (Terminal* terminal : instances) {
        if (terminal->tty == tty) {
            return terminal->_closedTty(command);
        }
    }
    CmdResult result;
    result.status = 1;
    result.message = "Terminal instance not found for tty " + to_string(tty) + "\n";
    return result;
}

CmdResult Terminal::_closedTty(const json& command) {
    (void)command;  
    CmdResult result;
    instances.erase(this);
    kvTable.deleteEntry(dirHistoryPointerKey);
    result.status = 0;
    result.message = "\n";
    delete this;
    return result;
}

string Terminal::dirHistoryEntryKey(int index) {
    return DIR_HISTORY_PREFIX + to_string(index);
}

CmdResult Terminal::updateDirHistory(const json& command) {
    int tty = command[TTY_KEY].get<int>();
    for (Terminal* terminal : instances) {
        if (terminal->tty == tty) {
            return terminal->_updateDirHistory(command);
        }
    }
    // Terminal not found, create it (auto-register on first updateDirHistory)
    Terminal* terminal = new Terminal(tty);
    return terminal->_updateDirHistory(command);
}

CmdResult Terminal::_updateDirHistory(const json& command) {
    CmdResult result;
    int index = getIndex();
    string pwd = getPWD(command);
    string currentDir = getDirHistoryEntry(index);
    string nextDir = getDirHistoryEntry(index+1);
    kvTable.upsert(INDEX_OF_LAST_TOUCHED_DIR_KEY, to_string(index));
    result.status = 0;
    result.message = "\n";
    if (pwd.empty()) {
        result.status = 1;
        result.message = "No directory provided to updateDirHistory\n";
        return result;
    }else if (currentDir == pwd) {
    }else if (nextDir == pwd) {
        kvTable.upsert(dirHistoryPointerKey, to_string(index+1));
    }else{
        int insertIndex = index + 1;
        kvTable.insertAt(dirHistoryKeyPrefix(), insertIndex, pwd);
        kvTable.upsert(dirHistoryPointerKey, to_string(insertIndex));
        kvTable.upsert(INDEX_OF_LAST_TOUCHED_DIR_KEY, to_string(index + 1));
    }
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
    return DIR_HISTORY_PREFIX;
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

CmdResult Terminal::showIndex(const json& command) {
    for (Terminal* terminal : instances) {
        if (terminal->tty == command[TTY_KEY].get<int>()) {
            CmdResult result;
            int index = terminal->getIndex();
            string dir = terminal->getDirHistoryEntry(index);
            result.message = "Index: " + to_string(index) + "\nDirectory: " + dir + "\n";
            result.status = 0;
            return result;
        }
    }
    CmdResult result;
    result.status = 1;
    result.message = "Terminal instance not found for tty " + to_string(command[TTY_KEY].get<int>()) + "\n";
    return result;
}

CmdResult Terminal::deleteAllDirEntries(const json& command) {
    (void)command;  // unused
    kvTable.deleteByPrefix(DIR_HISTORY_POINTER_PREFIX);
    kvTable.deleteByPrefix(DIR_HISTORY_PREFIX);
    // Reset to initial state
    kvTable.upsert(string(DIR_HISTORY_PREFIX) + "0", DIR_HISTORY_DEFAULT_DIR);
    kvTable.upsert(INDEX_OF_LAST_TOUCHED_DIR_KEY, "0");
    CmdResult result;
    result.status = 0;
    result.message = "All directory entries deleted\n";
    return result;
}

CmdResult Terminal::listAllEntries(const json& command) {
    (void)command;  // unused
    // List all dirHistory entries from database
    CmdResult result;
    result.message = "dirHistory entries:\n";
    
    bool foundAny = false;
    // Iterate through all possible indices and collect entries
    for (int i = 0; i < 1000; i++) {  // Assume max 1000 entries
        string entry = getDirHistoryEntry(i);
        if (entry.empty()) {
            if (i == 0) {
                // No entries at all
                result.message += "(empty)\n";
            }
            break;
        }
        foundAny = true;
        result.message += to_string(i) + ": " + entry + "\n";
    }
    
    if (!foundAny) {
        result.message = "No directory entries found\n";
    }
    
    result.status = 0;
    return result;
}
