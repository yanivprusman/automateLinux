#ifndef DIRHISTORY_H
#define DIRHISTORY_H

#include <string>
#include <fstream>

class DirHistory {
public:
    DirHistory();
    
    // File management - automatically uses Config for paths
    void resetWithDefaultDir();
    
    // TTY utilities
    std::string getEscapedTty(const std::string& filePath);
    
    // Validation - automatically uses Config for paths
    bool testIfProper();
    bool resetDirHistoryToBeginningStateIfError();
    
    // State management - automatically uses Config for paths
    void resetDirHistoryToBeginningState();
    
    void initializeDirHistory();
    
    // Navigation - automatically uses Config for paths
    void cdToPointer();
    
    // File manipulation - automatically uses Config for paths
    bool insertDir(const std::string& dir, int index, char sedCommand);
    bool insertDirAtIndex(const std::string& dir, int index);
    bool insertDirAfterIndex(const std::string& dir, int index);
    
    // Pointer management - automatically uses Config for paths
    void setDirHistoryPointer(int pointer, const std::string& tty = "");
    int getDirHistoryPointer(const std::string& tty = "");
    
    // History access - automatically uses Config for paths
    std::string getDirFromHistory();
    
    // History updates - automatically uses Config for paths
    void updateDirHistory();
    
    // Navigation operations - automatically uses Config for paths
    void navigateBack();
    void navigateForward();
    
private:
    std::string readLineFromFile(const std::string& filePath, int lineNumber);
    bool fileExists(const std::string& filePath);
    int countLines(const std::string& filePath);
};

#endif // DIRHISTORY_H