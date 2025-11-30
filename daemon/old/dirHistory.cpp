#include "dirHistory.h"
#include "config.h"
#include "system.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <regex>
#include <climits>
#include <unistd.h>

namespace fs = std::filesystem;

DirHistory::DirHistory() {}

void DirHistory::resetWithDefaultDir() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    std::string defaultDir = config->getDirHistoryDefaultDir();
    
    std::ofstream file(ttyFile, std::ios::trunc);
    if (file.is_open()) {
        file << defaultDir << std::endl;
        file.close();
    }
}

std::string DirHistory::getEscapedTty(const std::string& filePath) {
    Config* config = Config::getInstance();
    std::string fileBase = config->getDirHistoryFileBase();
    size_t lastSlash = filePath.find_last_of('/');
    std::string filename = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
    if (filename.length() > 3 && filename.substr(filename.length() - 3) == ".sh") {
        filename = filename.substr(0, filename.length() - 3);
    }
    if (filename.find(fileBase) == 0) {
        filename = filename.substr(fileBase.length());
    }
    return filename;
}

bool DirHistory::testIfProper() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    std::string pointersFile = config->getDirHistoryPointersFile();
    
    if (!fileExists(ttyFile)) {
        return false;
    }
    int totalLines = countLines(ttyFile);
    
    std::string escapedTty = config->getEscapedTty();
    int pointer = getDirHistoryPointer();
    
    if (pointer < 1 || pointer > totalLines) {
        return false;
    }
    
    if (!std::regex_match(std::to_string(pointer), std::regex("^[0-9]+$"))) {
        return false;
    }
    
    return true;
}

void DirHistory::resetDirHistoryToBeginningState() {
    Config* config = Config::getInstance();
    std::string dataDir = config->getDataDir();
    std::string ttyFile = config->getDirHistoryTtyFile();
    std::string pointersFile = config->getDirHistoryPointersFile();
    std::string escapedTty = config->getEscapedTty();
    
    std::string dirHistoryPath = dataDir + "dirHistory/";
    if (fs::exists(dirHistoryPath)) {
        for (const auto& entry : fs::directory_iterator(dirHistoryPath)) {
            fs::remove(entry.path());
        }
    }
    
    resetWithDefaultDir();
    setDirHistoryPointer(1);
}

void DirHistory::initializeDirHistory() {
    Config* config = Config::getInstance();
    std::string pointersFile = config->getDirHistoryPointersFile();
    std::string ttyFile = config->getDirHistoryTtyFile();
    std::string ttyFileBase = config->getDirHistoryFileBase();
    std::string escapedTty = config->getEscapedTty();
    std::string dataDir = config->getDataDir();
    
    if (!fileExists(pointersFile)) {
        resetDirHistoryToBeginningState();
        return;
    }
    
    std::string lastChanged;
    fs::file_time_type lastTime;
    bool foundFile = false;
    
    if (fs::exists(fs::path(ttyFileBase).parent_path())) {
        for (const auto& entry : fs::directory_iterator(fs::path(ttyFileBase).parent_path())) {
            std::string entryPath = entry.path().string();
            if (entryPath.find(ttyFileBase) == 0 && entryPath != pointersFile) {
                if (!foundFile || fs::last_write_time(entry) > lastTime) {
                    lastChanged = entryPath;
                    lastTime = fs::last_write_time(entry);
                    foundFile = true;
                }
            }
        }
    }
    
    if (!foundFile || !fileExists(lastChanged)) {
        resetDirHistoryToBeginningState();
        return;
    }
    
    if (lastChanged == ttyFile) {
        getDirHistoryPointer();
    } else {
        std::string tty = getEscapedTty(lastChanged);
        fs::copy_file(lastChanged, ttyFile, fs::copy_options::overwrite_existing);
        int pointer = getDirHistoryPointer(tty);
        setDirHistoryPointer(pointer);
    }
    
    if (!testIfProper()) {
        resetDirHistoryToBeginningState();
    }
}

void DirHistory::cdToPointer() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    int pointer = getDirHistoryPointer();
    
    if (fileExists(ttyFile) && pointer > 0) {
        std::string dir = readLineFromFile(ttyFile, pointer);
        if (!dir.empty() && fs::is_directory(dir)) {
            std::string command = "cd '" + dir + "' 2>/dev/null";
            std::cout << command << std::endl;
        }
    }
}

bool DirHistory::insertDir(const std::string& dir, int index, char sedCommand) {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    
    if (dir.empty() || index < 1) {
        return false;
    }
    
    if (fileExists(ttyFile) && fs::file_size(ttyFile) > 0) {
        std::string command;
        if (sedCommand == 'i') {
            command = "sed -i '" + std::to_string(index) + "i\\" + dir + "' '" + ttyFile + "'";
        } else if (sedCommand == 'a') {
            command = "sed -i '" + std::to_string(index) + "a\\" + dir + "' '" + ttyFile + "'";
        }
        System::executeBashCommand(command.c_str());
    } else {
        std::ofstream file(ttyFile, std::ios::trunc);
        if (file.is_open()) {
            file << dir << std::endl;
            file.close();
        } else {
            return false;
        }
    }
    return true;
}

bool DirHistory::insertDirAtIndex(const std::string& dir, int index) {
    return insertDir(dir, index, 'i');
}

bool DirHistory::insertDirAfterIndex(const std::string& dir, int index) {
    return insertDir(dir, index, 'a');
}

void DirHistory::navigateBack() {
    int pointer = getDirHistoryPointer();
    pointer--;
    if (pointer < 1) {
        pointer = 1;
    }
    setDirHistoryPointer(pointer);
    cdToPointer();
}

void DirHistory::navigateForward() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    
    if (fileExists(ttyFile)) {
        int totalLines = countLines(ttyFile);
        int pointer = getDirHistoryPointer();
        pointer++;
        if (pointer >= totalLines) {
            pointer = totalLines;
        }
        setDirHistoryPointer(pointer);
        cdToPointer();
    }
}

void DirHistory::setDirHistoryPointer(int pointer, const std::string& tty) {
    Config* config = Config::getInstance();
    std::string pointersFile = config->getDirHistoryPointersFile();
    std::string escapedTty = (tty.empty()) ? config->getEscapedTty() : tty;
    
    if (!fileExists(pointersFile)) {
        std::ofstream file(pointersFile);
        file.close();
    }
    
    std::ifstream inFile(pointersFile);
    std::string line;
    std::string content;
    bool found = false;
    
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string ttyStr;
        int ptr;
        if (iss >> ttyStr >> ptr) {
            if (ttyStr == escapedTty) {
                content += escapedTty + " " + std::to_string(pointer) + "\n";
                found = true;
            } else {
                content += line + "\n";
            }
        }
    }
    inFile.close();
    
    if (!found) {
        content += escapedTty + " " + std::to_string(pointer) + "\n";
    }
    
    std::ofstream outFile(pointersFile, std::ios::trunc);
    if (outFile.is_open()) {
        outFile << content;
        outFile.close();
    }
}

int DirHistory::getDirHistoryPointer(const std::string& tty) {
    Config* config = Config::getInstance();
    std::string pointersFile = config->getDirHistoryPointersFile();
    std::string escapedTty = (tty.empty()) ? config->getEscapedTty() : tty;
    
    std::ifstream file(pointersFile);
    if (!file.is_open()) return 0;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string ttyStr;
        int pointer;
        if (iss >> ttyStr >> pointer) {
            if (ttyStr == escapedTty) {
                return pointer;
            }
        }
    }
    return 0;
}

std::string DirHistory::getDirFromHistory() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    int pointer = getDirHistoryPointer();
    return readLineFromFile(ttyFile, pointer);
}

bool DirHistory::resetDirHistoryToBeginningStateIfError() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    std::string pointersFile = config->getDirHistoryPointersFile();
    
    if (!fileExists(ttyFile) || !fileExists(pointersFile)) {
        return false;
    }
    
    if (!testIfProper()) {
        return false;
    }
    
    return true;
}

void DirHistory::updateDirHistory() {
    Config* config = Config::getInstance();
    std::string ttyFile = config->getDirHistoryTtyFile();
    
    if (resetDirHistoryToBeginningStateIfError()) {
        int pointer = getDirHistoryPointer();
        std::string lastDir = getDirFromHistory();
        
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            return;
        }
        std::string currentDir(cwd);
        std::string dirWithTrailingSlash = currentDir + "/";
        
        if (dirWithTrailingSlash != lastDir) {
            insertDirAfterIndex(dirWithTrailingSlash, pointer);
            pointer++;
            setDirHistoryPointer(pointer);
        } else {
            std::string command = "touch '" + ttyFile + "'";
            System::executeBashCommand(command.c_str());
        }
    }
}

int DirHistory::countLines(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return 0;
    
    int lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineCount++;
    }
    return lineCount;
}

std::string DirHistory::readLineFromFile(const std::string& filePath, int lineNumber) {
    std::ifstream file(filePath);
    if (!file.is_open()) return "";
    
    std::string line;
    int currentLine = 1;
    while (std::getline(file, line)) {
        if (currentLine == lineNumber) {
            return line;
        }
        currentLine++;
    }
    return "";
}

bool DirHistory::fileExists(const std::string& filePath) {
    return fs::exists(filePath);
}