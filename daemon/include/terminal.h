#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

class Terminal {
public:
  Terminal(int tty);
  ~Terminal();
  static vector<Terminal *> instances;
  static Terminal *getInstanceByTTY(int tty);
  static CmdResult openedTty(const json &command);
  CmdResult _openedTty(const json &command);
  static CmdResult closedTty(const json &command);
  CmdResult _closedTty(const json &command);
  static CmdResult updateDirHistory(const json &command);
  CmdResult _updateDirHistory(const json &command);
  static CmdResult cdForward(const json &command);
  CmdResult _cdForward(const json &command);
  static CmdResult cdBackward(const json &command);
  CmdResult _cdBackward(const json &command);
  static CmdResult showTerminalInstance(const json &command);
  static CmdResult showAllTerminalInstances(const json &command);
  int tty;
  int getIndex();
  static string getPWD(const json &command);
  string getDirHistoryEntry(int index);
  string toString();
};

#endif // TERMINAL_H
