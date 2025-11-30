#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
private:
    static Config* instance;
    std::string automateLinuxDir;                              
    std::string pathEnd;                          
    std::string terminalDir;                      
    std::string envFile;                          
    std::string symlinkDir;                       
    std::string promptCommandScriptFile;          
    std::string terminalLogFile;                  
    std::string verbose;                          
    std::string dataDir;                          
    std::string dirHistoryDefaultDir;             
    std::string dirHistoryDir;                    
    std::string dirHistoryFileBase;               
    std::string escapedTty;                       
    std::string dirHistoryTtyFile;                
    std::string dirHistoryPointersFile;           
    std::string dirHistoryTestsDir;               
    std::string printBlockSeparator;              
    std::string terminalFunctionsDir;             
    std::string trapDir;                          
    std::string trapErrFile;                      
    std::string trapErrLogFile;                   
    std::string trapErrLogFileBackground;         
    std::string trapDebugFile;                    
    std::string trapGeneratorFile;                
    Config();
    void initializeConfig();
    std::string escapeTtyPath();
public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    static Config* getInstance();
    std::string getValue(const std::string& varName);
    std::string getDir() const { return automateLinuxDir; }
    std::string getPathEnd() const { return pathEnd; }
    std::string getTerminalDir() const { return terminalDir; }
    std::string getEnvFile() const { return envFile; }
    std::string getSymlinkDir() const { return symlinkDir; }
    std::string getPromptCommandScriptFile() const { return promptCommandScriptFile; }
    std::string getTerminalLogFile() const { return terminalLogFile; }
    std::string getVerbose() const { return verbose; }
    std::string getDataDir() const { return dataDir; }
    std::string getDirHistoryDefaultDir() const { return dirHistoryDefaultDir; }
    std::string getDirHistoryDir() const { return dirHistoryDir; }
    std::string getDirHistoryFileBase() const { return dirHistoryFileBase; }
    std::string getEscapedTty() const { return escapedTty; }
    std::string getDirHistoryTtyFile() const { return dirHistoryTtyFile; }
    std::string getDirHistoryPointersFile() const { return dirHistoryPointersFile; }
    std::string getDirHistoryTestsDir() const { return dirHistoryTestsDir; }
    std::string getPrintBlockSeparator() const { return printBlockSeparator; }
    std::string getTerminalFunctionsDir() const { return terminalFunctionsDir; }
    std::string getTrapDir() const { return trapDir; }
    std::string getTrapErrFile() const { return trapErrFile; }
    std::string getTrapErrLogFile() const { return trapErrLogFile; }
    std::string getTrapErrLogFileBackground() const { return trapErrLogFileBackground; }
    std::string getTrapDebugFile() const { return trapDebugFile; }
    std::string getTrapGeneratorFile() const { return trapGeneratorFile; }
};

#endif 
