#ifndef TERMINAL_H
#define TERMINAL_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sqlite3.h>
#include <ctime>

struct TTYState {
std::vector<std::string> history;
int pointer;
};

class Terminal {
public:
static Terminal& getInstance();
std::string ttyOpened(const std::string& tty);
std::string cdToPointer(const std::string& tty);
std::string updateDirHistory(const std::string& tty, const std::string& pwd);
std::string cdBackword(const std::string& tty);
std::string cdForword(const std::string& tty);
std::string ttyClosed(const std::string& tty);
static std::string getBaseDir();
private:
Terminal();
~Terminal();
sqlite3* db;
std::mutex mtx;
std::map<std::string, TTYState> cache;
void initializeDatabase();
std::string getLastChangedTTY();
std::string getLastChangedTTYNoLock();
void copyHistoryFromLastTTY(const std::string& currentTty);
void reloadCacheFromDB(const std::string& tty);
void writeCacheToDB(const std::string& tty);
};

#endif // TERMINAL_H
