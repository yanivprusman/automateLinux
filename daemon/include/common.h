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
#define DIR_HISTORY_ENTRY_PREFIX "dirHistory"
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
#define COMMAND_PING "ping"
#define COMMAND_QUIT "quit"
#define COMMAND_GET_KEYBOARD_PATH "getKeyboardPath"
#define COMMAND_SET_KEYBOARD "setKeyboard"
#define COMMAND_ARG_KEYBOARD_NAME "keyboardName"
#define PREFIX_KEYBOARD "corsairKeyBoardLogiMouse"
#define CODE_KEYBOARD "Code"
#define GNOME_TERMINAL_KEYBOARD "gnome-terminal-server"
#define GOOGLE_CHROME_KEYBOARD "google-chrome"
#define DEFAULT_KEYBOARD "DefaultKeyboard"
#define TEST_KEYBOARD "TestKeyboard"
#define KEYBOARD_DISCOVERY_CMD "ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd'"
#define KEYBOARD_INPUT_PATH "/dev/input/by-id/"
#define MOUSE_DISCOVERY_CMD "awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices"
#define MOUSE_INPUT_PATH "/dev/input/"
#define KEYBOARD_PATH_KEY "keyboardPath"
#define MOUSE_PATH_KEY "mousePath"
#define COMMAND_VALUE_TRUE "true"
#define COMMAND_VALUE_FALSE "false"
#define COMMAND_SHOULD_LOG "shouldLog"
#define COMMAND_TOGGLE_KEYBOARDS_WHEN_ACTIVE_WINDOW_CHANGES "toggleKeyboardsWhenActiveWindowChanges"
#define COMMAND_ARG_ENABLE "enable"
#define COMMAND_GET_DIR "getDir"
#define COMMAND_ARG_DIR_NAME "dirName"
#define COMMAND_GET_FILE "getFile"
#define COMMAND_ARG_FILE_NAME "fileName"
#define EVSIEVE_RANDOM_VAR "randomVar"
#define EVSIEVE_STANDARD_ERR_FILE "evsieveErr.log"
#define EVSIEVE_STANDARD_OUTPUT_FILE "evsieveOutput.log"

struct CommandSignature {
    string name;
    vector<string> requiredArgs;
    CommandSignature(const string& n, const vector<string>& args) 
        : name(n), requiredArgs(args) {}
};



struct Directories {
    string base;
    string data;
    string mappings;
    string symlinks;
    string terminal;
    Directories() {
        base = canonical("/proc/self/exe").parent_path().parent_path().string() + "/";
        data = base + "data/";
        mappings = base + "evsieve/mappings/";
        symlinks = base + "symlinks/";
        terminal = base + "terminal/";
    }
};

struct Files {
    struct File {
        string name;
        string dir;
        string fullPath() const { return dir + name; }
    };
    vector<File> files;
    void initialize(const Directories& dirs) {
        files = {
            {"chrome.log", dirs.data},
            {"combined.log", dirs.data},
            {"daemon.db", dirs.data},
            {"evsieveErr.log", dirs.data},
            {"evsieveOutput.log", dirs.data},
            {"trapErrLogBackground.txt", dirs.data},
            {"trapErrLog.txt", dirs.data},
            {"corsairKeyBoardLogiMouseCode.sh", dirs.mappings},
            {"corsairKeyBoardLogiMouseDefaultKeyboard.sh", dirs.mappings},
            {"corsairKeyBoardLogiMousegnome-terminal-server.sh", dirs.mappings},
            {"corsairKeyBoardLogiMousegoogle-chrome.sh", dirs.mappings},
            {"theRealPath.sh", dirs.terminal},
        };
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
extern Files& files;
extern string socketPath;
class KVTable;
extern KVTable& kvTable;
class DirHistory;
extern DirHistory& dirHistory;
extern std::ofstream g_logFile;

extern const CommandSignature COMMAND_REGISTRY[];
extern const size_t COMMAND_REGISTRY_SIZE;

#endif // COMMON_H

