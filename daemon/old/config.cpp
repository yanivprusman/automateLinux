#include "config.h"
#include "system.h"
#include <filesystem>
#include <stdexcept>

Config* Config::instance = nullptr;

Config::Config() {
    initializeConfig();
}

Config* Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return instance;
}

std::string Config::escapeTtyPath() {
    std::string tty = System::getTty();
    std::string escaped = tty;
    size_t pos = 0;
    while ((pos = escaped.find("/", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "_");
        pos += 1;
    }    
    return escaped;
}

void Config::initializeConfig() {
    automateLinuxDir = std::filesystem::canonical("/proc/self/exe").parent_path().parent_path().string();
    pathEnd = "/";
    automateLinuxDir += pathEnd;
    terminalDir = automateLinuxDir + "terminal" + pathEnd;
    envFile = terminalDir + "env.sh";
    symlinkDir = automateLinuxDir + "symlinks";
    promptCommandScriptFile = terminalDir + "promptCommand.sh";
    terminalLogFile = terminalDir + "terminalLog.txt";
    verbose = "true";
    dataDir = automateLinuxDir + "data/";
    dirHistoryDefaultDir = "/home/yaniv/coding/";
    dirHistoryDir = dataDir + "dirHistory/";
    dirHistoryFileBase = dirHistoryDir + "dirHistory";
    escapedTty = escapeTtyPath();
    dirHistoryTtyFile = dirHistoryFileBase + escapedTty + ".sh";
    dirHistoryPointersFile = dirHistoryDir + "dirHistoryPointers.sh";
    dirHistoryTestsDir = terminalDir + "tests/";
    printBlockSeparator = "------------------------------------------";
    terminalFunctionsDir = terminalDir + "functions/";
    trapDir = terminalDir + "trap/";
    trapErrFile = trapDir + "err.sh";
    trapErrLogFile = dataDir + "trapErrLog.txt";
    trapErrLogFileBackground = dataDir + "trapErrLogBackground.txt";
    trapDebugFile = trapDir + "debug.sh";
    trapGeneratorFile = trapDir + "trapGenerator.sh";
}

std::string Config::getValue(const std::string& varName) {
    if (varName == "AUTOMATE_LINUX_DIR") {
        return automateLinuxDir;
    } else if (varName == "PATH_END") {
        return pathEnd;
    } else if (varName == "TERMINAL_DIR") {
        return terminalDir;
    } else if (varName == "ENV_FILE") {
        return envFile;
    } else if (varName == "SYMLINK_DIR") {
        return symlinkDir;
    } else if (varName == "PROMPT_COMMAND_SCRIPT_FILE") {
        return promptCommandScriptFile;
    } else if (varName == "TERMINAL_LOG_FILE") {
        return terminalLogFile;
    } else if (varName == "VERBOSE") {
        return verbose;
    } else if (varName == "DATA_DIR") {
        return dataDir;
    } else if (varName == "DIR_HISTORY_DEFAULT_DIR") {
        return dirHistoryDefaultDir;
    } else if (varName == "DIR_HISTORY_DIR") {
        return dirHistoryDir;
    } else if (varName == "DIR_HISTORY_FILE_BASE") {
        return dirHistoryFileBase;
    } else if (varName == "ESCAPED_TTY") {
        return escapedTty;
    } else if (varName == "DIR_HISTORY_TTY_FILE") {
        return dirHistoryTtyFile;
    } else if (varName == "DIR_HISTORY_POINTERS_FILE") {
        return dirHistoryPointersFile;
    } else if (varName == "DIR_HISTORY_TESTS_DIR") {
        return dirHistoryTestsDir;
    } else if (varName == "PRINT_BLOCK_SEPARATOR") {
        return printBlockSeparator;
    } else if (varName == "TERMINAL_FUNCTIONS_DIR") {
        return terminalFunctionsDir;
    } else if (varName == "TRAP_DIR") {
        return trapDir;
    } else if (varName == "TRAP_ERR_FILE") {
        return trapErrFile;
    } else if (varName == "TRAP_ERR_LOG_FILE") {
        return trapErrLogFile;
    } else if (varName == "TRAP_ERR_LOG_FILE_BACKGROUND") {
        return trapErrLogFileBackground;
    } else if (varName == "TRAP_DEBUG_FILE") {
        return trapDebugFile;
    } else if (varName == "TRAP_GENERATOR_FILE") {
        return trapGeneratorFile;
    }
    
    // Unknown variable
    return "";
}
