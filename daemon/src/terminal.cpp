#include "terminal.h"
#include "DatabaseTableManagers.h"
#include "Utils.h"

vector<Terminal *> Terminal::instances;

string Terminal::standardizePath(string path) {
  if (path.empty())
    return "/";
  if (path.back() != '/') {
    path += "/";
  }
  return path;
}

Terminal::Terminal(int tty) : tty(tty) {
  instances.push_back(this);
  int lastIndex = TerminalTable::getMaxHistoryIndex();
  if (lastIndex < 0) {
    lastIndex = 0;
    string defaultDir = standardizePath(DIR_HISTORY_DEFAULT_DIR);
    TerminalTable::upsertHistory(0, defaultDir);
  }
  TerminalTable::setSessionPointer(tty, lastIndex);
}

Terminal::~Terminal() {
  auto it = std::find(instances.begin(), instances.end(), this);
  if (it != instances.end()) {
    instances.erase(it);
  }
  TerminalTable::deleteSession(tty);
}

Terminal *Terminal::getInstanceByTTY(int tty) {
  for (Terminal *terminal : instances) {
    if (terminal->tty == tty) {
      return terminal;
    }
  }
  return nullptr;
}

static string getJsonString(const json &j, const string &key) {
  if (!j.contains(key))
    return "";
  if (j[key].is_string())
    return j[key].get<string>();
  if (j[key].is_number()) {
    try {
      return to_string(j[key].get<long long>());
    } catch (...) {
      return to_string(j[key].get<double>());
    }
  }
  return "";
}

CmdResult Terminal::openedTty(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int ttyInt = std::stoi(ttyStr);
  Terminal *terminal = getInstanceByTTY(ttyInt);
  if (terminal) {
    // If instance exists, we might need to reset or just redirect.
    // For now, let's just use it.
    return terminal->_openedTty(command);
  }
  terminal = new Terminal(ttyInt);
  return terminal->_openedTty(command);
}

CmdResult Terminal::_openedTty(const json &command) {
  (void)command;
  CmdResult result;
  try {
    int index = TerminalTable::getSessionPointer(tty);
    if (index < 0) {
      index = 0;
      TerminalTable::setSessionPointer(tty, 0);
      string defaultDir = standardizePath(DIR_HISTORY_DEFAULT_DIR);
      TerminalTable::upsertHistory(0, defaultDir);
    }
    string dir = standardizePath(TerminalTable::getHistory(index));
    if (dir.empty() || dir == "/") {
      dir = standardizePath(DIR_HISTORY_DEFAULT_DIR);
      TerminalTable::upsertHistory(index, dir);
    }
    result.message = dir + mustEndWithNewLine;
    result.status = 0;
  } catch (const std::exception &e) {
    result.status = 1;
    result.message = "Error in openedTty: " + std::string(e.what()) + "\n";
  }
  return result;
}

CmdResult Terminal::closedTty(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int tty = std::stoi(ttyStr);
  Terminal *terminal = getInstanceByTTY(tty);
  if (terminal) {
    return terminal->_closedTty(command);
  }
  CmdResult result;
  result.status = 1;
  result.message = string("Terminal instance not found for tty ") + ttyStr +
                   mustEndWithNewLine;
  return result;
}

CmdResult Terminal::_closedTty(const json &command) {
  (void)command;
  CmdResult result;
  auto it = std::find(instances.begin(), instances.end(), this);
  if (it != instances.end()) {
    instances.erase(it);
  }
  TerminalTable::deleteSession(tty);
  result.status = 0;
  result.message = "\n";
  delete this;
  return result;
}

// removed dirHistoryEntryKey

CmdResult Terminal::updateDirHistory(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int tty = std::stoi(ttyStr);
  Terminal *terminal = getInstanceByTTY(tty);
  if (terminal) {
    return terminal->_updateDirHistory(command);
  }
  // Terminal not found, create it (auto-register on first updateDirHistory)
  terminal = new Terminal(tty);
  return terminal->_updateDirHistory(command);
}

CmdResult Terminal::_updateDirHistory(const json &command) {
  CmdResult result;
  int index = getIndex();
  string pwd = standardizePath(getPWD(command));
  string currentDir = standardizePath(getDirHistoryEntry(index));

  // check next entry to see if we're just moving forward in existing history
  string nextDir = standardizePath(getDirHistoryEntry(index + 1));

  forceLog("[Terminal] updateDirHistory tty=" + to_string(tty) +
           " index=" + to_string(index) + " pwd=" + pwd + " cur=" + currentDir +
           " next=" + nextDir);

  SettingsTable::setSetting(INDEX_OF_LAST_TOUCHED_DIR_KEY, to_string(index));
  result.status = 0;
  result.message = "\n";
  if (pwd.empty() || pwd == "/") {
    result.status = 1;
    result.message = "No directory provided to updateDirHistory\n";
    return result;
  } else if (currentDir == pwd) {
    // already there
  } else if (nextDir == pwd) {
    TerminalTable::setSessionPointer(tty, index + 1);
  } else {
    int insertIndex = TerminalTable::getMaxHistoryIndex() + 1;
    TerminalTable::upsertHistory(insertIndex, pwd);
    TerminalTable::setSessionPointer(tty, insertIndex);
    SettingsTable::setSetting(INDEX_OF_LAST_TOUCHED_DIR_KEY,
                              to_string(insertIndex));
  }
  return result;
}

int Terminal::getIndex() {
  int idx = TerminalTable::getSessionPointer(tty);
  return idx >= 0 ? idx : 0;
}

string Terminal::getPWD(const json &command) {
  return command[PWD_KEY].get<string>();
}

string Terminal::getDirHistoryEntry(int index) {
  return TerminalTable::getHistory(index);
}

// removed dirHistoryKeyPrefix

CmdResult Terminal::cdForward(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int tty = std::stoi(ttyStr);
  Terminal *terminal = getInstanceByTTY(tty);
  if (!terminal) {
    terminal = new Terminal(tty);
  }
  return terminal->_cdForward(command);
}

CmdResult Terminal::_cdForward(const json &command) {
  (void)command;
  CmdResult result;
  int index = getIndex();
  int maxIndex = TerminalTable::getMaxHistoryIndex();

  forceLog("[Terminal] cdForward tty=" + to_string(tty) +
           " index=" + to_string(index) + " max=" + to_string(maxIndex));

  if (index >= maxIndex) {
    result.status = 0;
    result.message = string ("echo \nEND OF HISTORY reached\n") + mustEndWithNewLine;
    return result;
  }

  int nextIndex = index + 1;
  string nextDir = standardizePath(getDirHistoryEntry(nextIndex));

  if (nextDir.empty() || nextDir == "/") {
    // Safety fallback if there's a gap or error
    result.status = 1;
    result.message = "echo 'Empty history entry at " + to_string(nextIndex) +
                     "'; cd ." + mustEndWithNewLine;
    return result;
  }

  TerminalTable::setSessionPointer(tty, nextIndex);
  result.message = "cd " + nextDir + mustEndWithNewLine;
  result.status = 0;
  return result;
}

CmdResult Terminal::cdBackward(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int tty = std::stoi(ttyStr);
  Terminal *terminal = getInstanceByTTY(tty);
  if (!terminal) {
    terminal = new Terminal(tty);
  }
  return terminal->_cdBackward(command);
}

CmdResult Terminal::_cdBackward(const json &command) {
  (void)command;
  CmdResult result;
  int index = getIndex();

  forceLog("[Terminal] cdBackward tty=" + to_string(tty) +
           " index=" + to_string(index));

  if (index <= 0) {
    result.status = 0;
    string currentDir = standardizePath(getDirHistoryEntry(0));
    if (currentDir.empty() || currentDir == "/")
      currentDir = standardizePath(DIR_HISTORY_DEFAULT_DIR);
    result.message = string("echo \nBEGINNING OF HISTORY reached\n") + mustEndWithNewLine;
    return result;
  }

  int prevIndex = index - 1;
  string prevDir = standardizePath(getDirHistoryEntry(prevIndex));

  if (prevDir.empty() || prevDir == "/") {
    result.status = 1;
    result.message = "echo 'Empty history entry at " + to_string(prevIndex) +
                     "'; cd ." + mustEndWithNewLine;
    return result;
  }

  TerminalTable::setSessionPointer(tty, prevIndex);
  result.message = "cd " + prevDir + mustEndWithNewLine;
  result.status = 0;
  return result;
}

string Terminal::toString() {
  int index = getIndex();
  string dir = getDirHistoryEntry(index);
  return "tty: " + to_string(tty) + "\nDir history index: " + to_string(index) +
         "\ncwd: " + dir + mustEndWithNewLine;
}

CmdResult Terminal::showTerminalInstance(const json &command) {
  string ttyStr = getJsonString(command, TTY_KEY);
  if (ttyStr.empty())
    return CmdResult(1, "Missing tty argument\n");
  int tty = std::stoi(ttyStr);
  CmdResult result;
  Terminal *terminal = getInstanceByTTY(tty);
  if (terminal) {
    result.message = terminal->toString();
    result.status = 0;
    return result;
  }
  result.status = 1;
  result.message = string("Terminal instance not found for tty ") + ttyStr +
                   mustEndWithNewLine;
  return result;
}

CmdResult Terminal::showAllTerminalInstances(const json &command) {
  (void)command;
  CmdResult result;
  result.status = 0;
  if (instances.empty()) {
    result.message = "No terminal instances found\n";
    return result;
  }
  for (Terminal *terminal : instances) {
    result.message += terminal->toString() + "\n";
  }
  result.message.pop_back();
  return result;
}
