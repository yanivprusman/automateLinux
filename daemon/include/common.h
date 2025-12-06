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

#define OUTPUT_FILE "/home/yaniv/coding/automateLinux/daemon/output.txt"
#define SOCKET_PATH "/run/automatelinux/automatelinux-daemon.sock"
#define DIR_HISTORY_POINTER_PREFIX "pointerDevPts"
#define INDEX_OF_LAST_TOUCHED_DIR_KEY "indexOfLastTouchedDir"
#define TTY_KEY "tty"
#define PWD_KEY "pwd"
#define DIR_HISTORY_PREFIX "dirHistory"
#define DIR_HISTORY_DEFAULT_DIR "/home/yaniv/coding/"
#define COMMAND_KEY "command"
#define COMMAND_OPENED_TTY "openedTty"
#define COMMAND_CLOSED_TTY "closedTty"
#define COMMAND_UPDATE_DIR_HISTORY "updateDirHistory"
#define COMMAND_DELETE_ENTRY "deleteEntry"
#define COMMAND_CDFORWARD "cdForward"
#define COMMAND_CDBACKWARD "cdBackward"
#define COMMAND_SHOW_INDEX "showIndex"
#define COMMAND_DELETE_ALL_DIR_ENTRIES "deleteAllDirEntries"
#define COMMAND_LIST_ALL_ENTRIES "listAllEntries"
#define mustEndWithNewLine "\n"

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
    CmdResult(int s = 0, const std::string& msg = "\n") 
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

// inline std::string wrapJsonResponse(const std::string& message, int status = 0) {
//     json response;
//     if (isMultiline(message)) {
//         response["message"] = toJsonSingleLine(message);
//     } else {
//         response["message"] = message;
//     }
//     response["status"] = status;
//     return response.dump() + "\n";
// }

extern Directories& directories;
extern string socketPath;
class KVTable;
extern KVTable& kvTable;
class DirHistory;
extern DirHistory& dirHistory;


#endif // COMMON_H

