#ifndef COMMON_H
#define COMMON_H

#include "using.h"
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <systemd/sd-daemon.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/stat.h>
#include <sstream>
#include <cerrno>
#include <sqlite3.h>
#include <utility>

#define OUTPUT_FILE "/home/yaniv/coding/automateLinux/daemon/output.txt"
#define SOCKET_PATH "/run/automatelinux/automatelinux-daemon.sock"
#define DIR_HISTORY_POINTER_PREFIX "pointerDevPts"
#define INDEX_OF_LAST_TOUCHED_DIR_KEY "indexOfLastTouchedDir"
#define TTY_KEY "tty"
#define PWD_KEY "pwd"
#define DIR_HISTORY_PREFIX "dirHistory"
#define DIR_HISTORY_DEFAULT_DIR "/home/yaniv/coding/"
#define mustEndWithNewLine "\n"
#define COMMAND_KEY "command"
#define COMMAND_EMPTY ""
#define COMMAND_HELP_DDASH "--help"
#define COMMAND_OPENED_TTY "openedTty"
#define COMMAND_CLOSED_TTY "closedTty"
#define COMMAND_UPDATE_DIR_HISTORY "updateDirHistory"
#define COMMAND_CD_FORWARD "cdForward"
#define COMMAND_CD_BACKWARD "cdBackward"
#define COMMAND_SHOW_TERMINAL_INSTANCE "showTerminalInstance"
#define COMMAND_SHOW_ALL_TERMINAL_INSTANCES "showAllTerminalInstances"
#define COMMAND_DELETE_ENTRY "deleteEntry"
#define COMMAND_DELETE_ENTRIES_BY_PREFIX "deleteEntriesByPrefix"
#define COMMAND_SHOW_ENTRIES_BY_PREFIX "showEntriesByPrefix"
#define COMMAND_SHOW_DB "showDB"
#define COMMAND_PRINT_DIR_HISTORY "printDirHistory"
#define COMMAND_UPSERT_ENTRY "upsertEntry"
#define COMMAND_GET_ENTRY "getEntry"
#define COMMAND_ARG_TTY "tty"
#define COMMAND_ARG_PWD "pwd"
#define COMMAND_ARG_KEY "key"
#define COMMAND_ARG_PREFIX "prefix"
#define COMMAND_ARG_VALUE "value"

struct CommandSignature {
    string name;
    vector<string> requiredArgs;
    CommandSignature(const string& n, const vector<string>& args) 
        : name(n), requiredArgs(args) {}
};

static const CommandSignature COMMAND_REGISTRY[] = {
    CommandSignature(COMMAND_EMPTY, {}),
    CommandSignature(COMMAND_HELP_DDASH, {}),
    CommandSignature(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_UPDATE_DIR_HISTORY, {COMMAND_ARG_TTY, COMMAND_ARG_PWD}),
    CommandSignature(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHOW_TERMINAL_INSTANCE , {COMMAND_ARG_TTY}),
    CommandSignature(COMMAND_SHOW_ALL_TERMINAL_INSTANCES , {}),
    CommandSignature(COMMAND_DELETE_ENTRY, {COMMAND_ARG_KEY}),
    CommandSignature(COMMAND_SHOW_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_DELETE_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}),
    CommandSignature(COMMAND_SHOW_DB, {}),
    CommandSignature(COMMAND_PRINT_DIR_HISTORY, {}),
    CommandSignature(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),
    CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}),
};

static const size_t COMMAND_REGISTRY_SIZE = sizeof(COMMAND_REGISTRY) / sizeof(COMMAND_REGISTRY[0]);

struct Directories {
    string base;
    string data;
    Directories() {
        base = canonical("/proc/self/exe").parent_path().parent_path().string() + "/";
        data = base + "data/";
    }
};

struct CmdResult {
    int status;         
    std::string message;
    CmdResult(int s = 0, const std::string& msg = "") 
        : status(s), message(msg) {}
};

struct Timer {
    std::chrono::high_resolution_clock::time_point start;
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    double elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        return duration.count();
    }
    std::string elapsedStr() const {
        return std::to_string(elapsed()) + " seconds";
    }
};

inline bool isMultiline(const std::string& s) {
    if (s.empty()) return false;
    size_t end = (s.back() == '\n') ? s.size() - 1 : s.size();
    return s.find('\n') < end;
}

inline std::string toJsonSingleLine(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            default: out += c;
        }
    }
    return out;
}

extern Directories& directories;
extern string socketPath;
class KVTable;
extern KVTable& kvTable;
class DirHistory;
extern DirHistory& dirHistory;


#endif // COMMON_H

