#include "cmdTerminal.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "MySQLManager.h"
#include "terminal.h"
#include <sstream>

using namespace std;

CmdResult handleOpenedTty(const json &command) {
  return Terminal::openedTty(command);
}

CmdResult handleClosedTty(const json &command) {
  return Terminal::closedTty(command);
}

CmdResult handleUpdateDirHistory(const json &command) {
  return Terminal::updateDirHistory(command);
}

CmdResult handleCdForward(const json &command) {
  return Terminal::cdForward(command);
}

CmdResult handleCdBackward(const json &command) {
  return Terminal::cdBackward(command);
}

CmdResult handleShowTerminalInstance(const json &command) {
  return Terminal::showTerminalInstance(command);
}

CmdResult handleShowAllTerminalInstances(const json &command) {
  return Terminal::showAllTerminalInstances(command);
}

CmdResult handlePrintDirHistory(const json &) {
  std::stringstream ss;
  ss << "--- Directory History Entries ---\n";
  auto history = TerminalTable::getAllHistory();
  if (history.empty()) {
    ss << "No directory entries found.\n";
  } else {
    for (const auto &t : history) {
      ss << "  TTY " << std::get<0>(t) << " Index " << std::get<1>(t) << ": "
         << std::get<2>(t) << "\n";
    }
  }

  ss << "\n--- Terminal Sessions ---\n";
  auto sessions = TerminalTable::getAllSessions();
  if (sessions.empty()) {
    ss << "No active sessions found.\n";
  } else {
    for (const auto &pair : sessions) {
      ss << "  TTY " << pair.first << ": Index " << pair.second << "\n";
    }
  }

  ss << "\n--- Last Touched Directory ---\n";
  string lastTouchedIndex =
      SettingsTable::getSetting(INDEX_OF_LAST_TOUCHED_DIR_KEY);
  if (!lastTouchedIndex.empty()) {
    ss << "  Index: " << lastTouchedIndex
       << " (lookup disabled due to per-TTY schema)\n";
  } else {
    ss << "No last touched directory recorded.\n";
  }

  return CmdResult(0, ss.str());
}

CmdResult handleEmptyDirHistoryTable(const json &) {
  MySQLManager::emptyTable("terminal_history");
  return CmdResult(0, "terminal_history table emptied.\n");
}

CmdResult handleShellSignal(const json &command) {
  string signal = command[COMMAND_ARG_SIGNAL].get<string>();
  // We return a bash command that the shell will eval.
  // This allows the shell to signal itself (e.g. SIGWINCH)
  // after the bind -x call completes.
  string bashCmd = "kill -" + signal + " $$" + mustEndWithNewLine;
  return CmdResult(0, bashCmd);
}
