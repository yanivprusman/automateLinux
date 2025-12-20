# GEMINI.md - Project Overview

This document provides an overview of the `automateLinux` project, a suite of custom automations designed to enhance and customize a Linux desktop environment. The project integrates various components, including a C++ daemon for system-level operations, a comprehensive set of shell scripts for terminal customization, and `evsieve` configurations for input event remapping.

## Project Overview

The `automateLinux` project aims to provide a highly personalized and automated Linux experience. Its core components work in conjunction to manage system resources, streamline workflows, and enable advanced input device control.

*   **C++ Daemon (`daemon/`)**: A central background service responsible for system-level tasks. It discovers and manages input device paths (keyboard, mouse), maintains a key-value store (`KVTable`), handles directory history, and facilitates inter-process communication via UNIX domain sockets. It exposes a command interface for clients.
*   **Terminal Environment Customization (`terminal/`)**: A modular collection of Bash scripts that extensively customize the user's shell environment. This includes setting environment variables, defining aliases and functions, configuring key bindings, managing the prompt (`PS1`, `PROMPT_COMMAND`), and enabling dynamic behaviors within the terminal.
*   **Input Event Handling (`evsieve/`)**: Utilizes the `evsieve` tool to intercept, remap, and act upon input events from various devices. This allows for complex key rebindings, execution of shell commands based on input patterns, and other advanced input-related automations.
*   **GNOME Shell Extensions (`gnomeExtensions/`)**: A collection of extensions that provide desktop integration. These extensions leverage a shared library (`gnomeExtensions/lib/`) for common functionality:
    *   `logging.js`: Provides a standardized logging class for consistent log output across all extensions.
    *   `daemon.js`: Provides a `DaemonConnector` class that centralizes the logic for communicating with the C++ daemon via its UNIX domain socket.
    *   `shellCommand.js`: Provides a `ShellCommandExecutor` class to asynchronously execute shell commands directly from extensions, used for system-level actions like shutdown.

    The `clock@ya-niv.com` extension now features:
    *   **Position Persistence**: The clock's position is saved via the C++ daemon (`KVTable`) and reloaded upon extension enablement, ensuring it retains its last-known position across sessions.
    *   **Right-Click Menu**: A right-click context menu has been added to the clock label, offering a "Shut Down" option that directly executes a system power-off command (`/usr/bin/systemctl poweroff`).
*   **Desktop Integration (`applications/`, `autostart/`, `desktop/`)**: Contains `.desktop` files for application launchers, autostart configurations, and custom Gnome Shell extensions to integrate automations directly into the graphical desktop environment.

## Building and Running

### C++ Daemon

The daemon is a C++ application built using CMake and Make. It can operate as a server or accept commands as a client.

*   **Build**:
    The daemon can be built using the `build.sh` script located in the `daemon/` directory. This script handles CMake configuration, compilation, and post-build steps, including restarting the systemd service.
    ```bash
    # To build and deploy the daemon:
    bd
    ```
*   **Running the Daemon (Server Mode)**:
    The daemon is designed to run as a systemd service (`daemon.service`). The `build.sh` script typically restarts this service upon successful compilation.
*   **Sending Commands to the Daemon (Client Mode)**:
    The `daemon` executable is available in the system's PATH and can act as a client to send JSON commands to a running daemon instance via its UNIX socket.
    ```bash
    # Example: Send a 'ping' command
    daemon send ping

    # Example: Send a command with arguments
    daemon send setKeyboard --keyboardName Code
    ```
    Commands are parsed from command-line arguments using a `--key value` format.

*   **Discovering Available Commands**:
    Before adding new functionality, it's crucial to check for existing daemon commands to avoid errors like `Unknown command`. The C++ source code is the definitive source of truth.

    *   **Primary Source**: The `COMMAND_REGISTRY` array in `daemon/src/mainCommand.cpp` provides a complete list of all registered commands and their required arguments. For example:
        ```cpp
        const CommandSignature COMMAND_REGISTRY[] = {
            // ...
            CommandSignature(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),
            CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}),
            // ...
        };
        ```
    *   **Command Definitions**: The string literals for commands (e.g., `COMMAND_UPSERT_ENTRY`) are defined as macros in `daemon/include/common.h`. This file is useful for cross-referencing macro names with their string values.

    By consulting these files, you can construct valid JSON commands to send to the daemon. For instance, to use `upsertEntry`, you know you must provide a `key` and a `value`.

### Terminal Environment

The terminal customizations are applied by sourcing several Bash scripts.

*   **Integration**:
    To integrate these customizations into your shell, ensure your `~/.bashrc` sources the project's main `terminal/bashrc` file. This file, in turn, sources other modular scripts.
    ```bash
    # In ~/.bashrc:
    if [ -f "/path/to/automateLinux/terminal/bashrc" ]; then
        . "/path/to/automateLinux/terminal/bashrc"
    fi
    ```
*   **Key Scripts**:
    *   `terminal/firstThing.sh`: Initializes core `AUTOMATE_LINUX_` environment variables, including paths to the daemon's socket and data directories.
    *   `terminal/myBashrc.sh`: Sources additional configuration scripts for aliases, functions, bindings, and manages the shell prompt (`PS1`, `PROMPT_COMMAND`).

### `evsieve` Configurations

`evsieve` is used for advanced input event remapping. Specific configurations are found in the `evsieve/mappings/` directory.

*   **Usage**:
    The `evsieve` instructions provide examples for setting up input hooks, remapping keys, and executing shell commands based on input. These configurations are typically run as separate processes to apply the desired event transformations.
    ```bash
    # Example from evsieve/instructions.txt:
    # Print direct input events:
    sudo evsieve --input /dev/input/event* --print format=direct

    # Remap Ctrl+S to Ctrl+N (illustrative, actual implementation involves more parameters)
    # This involves setting up hooks and send-key commands as described in evsieve documentation.

## Development Conventions

*   **Modularity**: The project emphasizes modularity, particularly within the `terminal/` scripts, where concerns like aliases, functions, and environment variables are separated into distinct files.
*   **Shared Extension Modules**: For GNOME Shell extensions, common functionality (like logging and daemon communication) is centralized in shared modules within `gnomeExtensions/lib/` to promote code reuse and simplify maintenance.
*   **Environment Variables**: Extensive use of `AUTOMATE_LINUX_` prefixed environment variables for managing paths, configurations, and inter-script communication.
*   **Daemon-Client Architecture**: A clear client-server model for system services, leveraging UNIX sockets for efficient and secure inter-process communication.
*   **Input Handling**: A significant focus on intercepting and manipulating input events, utilizing both the C++ daemon's input device discovery and `evsieve` for event remapping.
*   **Systemd Integration**: The C++ daemon is managed as a `systemd` service, ensuring it starts automatically and is properly managed by the operating system.
*   **External Dependencies**: The C++ daemon relies on `SQLite3` for data storage and `systemd` for service management. `evsieve` is an external tool used for input event processing.


in case we want to add a command to daemon instructions on how to add a command are provided in /home/yaniv/coding/automateLinux/daemon/AGENTS.md


how can i split the mainCommand.cpp?
Current Problems with mainCommand.cpp

Too many responsibilities: Command parsing, validation, handling, execution, logging, script execution
1000+ lines in a single file
Hard to test individual handlers
Tight coupling between command dispatch and implementation

Proposed Structure
src/
├── commands/
│   ├── command_types.h          // Command signatures & constants
│   ├── command_registry.h/cpp   // Command registration
│   ├── command_validator.h/cpp  // Validation logic
│   ├── command_dispatcher.h/cpp // Main dispatch logic
│   └── handlers/
│       ├── handler_interface.h  // Base handler interface
│       ├── terminal_handler.h/cpp
│       ├── database_handler.h/cpp
│       ├── system_handler.h/cpp
│       ├── keyboard_handler.h/cpp
│       └── info_handler.h/cpp
└── utils/
    ├── logger.h/cpp
    └── script_executor.h/cpp
actually lets put all .h files in the include/ folder and all .cpp files in the src/ folder
here are the files:
command_types.h name it CommandTypes.h  Command definitions only
#ifndef COMMAND_TYPES_H
#define COMMAND_TYPES_H

#include "common.h"

// Command result structure
struct CmdResult {
    int status;
    std::string message;
    
    CmdResult(int s = 0, const std::string& msg = "") 
        : status(s), message(msg) {}
    
    static CmdResult success(const std::string& msg = "") {
        return CmdResult(0, msg);
    }
    
    static CmdResult error(const std::string& msg, int code = 1) {
        return CmdResult(code, msg);
    }
};

// Command signature definition
struct CommandSignature {
    string name;
    vector<string> requiredArgs;
    
    CommandSignature(const string& n, const vector<string>& args) 
        : name(n), requiredArgs(args) {}
};

// Forward declaration
class ICommandHandler;

// Command definition with handler
struct CommandDefinition {
    CommandSignature signature;
    ICommandHandler* handler;
    
    CommandDefinition(const CommandSignature& sig, ICommandHandler* h)
        : signature(sig), handler(h) {}
};

#endif // COMMAND_TYPES_H

handler_interface.h name it HandlerInterface.h Base handler interface
#ifndef HANDLER_INTERFACE_H
#define HANDLER_INTERFACE_H

#include "command_types.h"

// Base interface for all command handlers
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual CmdResult handle(const json& command, int client_sock) = 0;
};

// Helper base class with common utilities
class BaseHandler : public ICommandHandler {
protected:
    // Common helper methods
    static string formatEntriesAsText(const vector<std::pair<string, string>>& entries) {
        if (entries.empty()) {
            return "<no entries>\n";
        }
        string result;
        for (const auto& pair : entries) {
            result += pair.first + "|" + pair.second + "\n";
        }
        return result;
    }
    
    static string errorEntryNotFound(const string& key) {
        return "Entry not found for key " + key + "\n";
    }
    
    static string errorDeleteFailed(const string& prefix) {
        return "Error deleting entries with prefix: " + prefix + "\n";
    }
};

#endif // HANDLER_INTERFACE_H

terminal_handler.h/cpp call it TerminalHandler.h/cpp - All terminal-related commands
#ifndef TERMINAL_HANDLER_H
#define TERMINAL_HANDLER_H

#include "handler_interface.h"
#include "terminal.h"

class OpenedTtyHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::openedTty(command);
    }
};

class ClosedTtyHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::closedTty(command);
    }
};

class UpdateDirHistoryHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::updateDirHistory(command);
    }
};

class CdForwardHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::cdForward(command);
    }
};

class CdBackwardHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::cdBackward(command);
    }
};

class ShowTerminalInstanceHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::showTerminalInstance(command);
    }
};

class ShowAllTerminalInstancesHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        return Terminal::showAllTerminalInstances(command);
    }
};

#endif // TERMINAL_HANDLER_H

database_handler.h call it DatabaseHandler.h - All database operations
#ifndef DATABASE_HANDLER_H
#define DATABASE_HANDLER_H

#include "handler_interface.h"
#include "KVTable.h"

class ShowEntriesByPrefixHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string prefix = command[COMMAND_ARG_PREFIX].get<string>();
        auto entries = kvTable.getByPrefix(prefix);
        return CmdResult::success(formatEntriesAsText(entries));
    }
};

class DeleteEntriesByPrefixHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string prefix = command[COMMAND_ARG_PREFIX].get<string>();
        int rc = kvTable.deleteByPrefix(prefix);
        if (rc == SQLITE_OK) {
            return CmdResult::success("Entries deleted\n");
        }
        return CmdResult::error(errorDeleteFailed(prefix));
    }
};

class ShowDbHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        return CmdResult::success(formatEntriesAsText(kvTable.getAll()));
    }
};

class DeleteEntryHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string key = command[COMMAND_ARG_KEY].get<string>();
        int rc = kvTable.deleteEntry(key);
        if (rc == SQLITE_OK) {
            return CmdResult::success("Entry deleted\n");
        }
        return CmdResult::error(errorEntryNotFound(key));
    }
};

class UpsertEntryHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string key = command[COMMAND_ARG_KEY].get<string>();
        string value = command[COMMAND_ARG_VALUE].get<string>();
        int rc = kvTable.upsert(key, value);
        if (rc == SQLITE_OK) {
            return CmdResult::success("Entry upserted\n");
        }
        string errorMsg = "Upsert failed with code " + to_string(rc) + 
                         ": " + sqlite3_errstr(rc) + "\n";
        return CmdResult::error(errorMsg, rc);
    }
};

class GetEntryHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string key = command[COMMAND_ARG_KEY].get<string>();
        string value = kvTable.get(key);
        if (value.empty()) {
            return CmdResult::error("\n");
        }
        return CmdResult::success(value + "\n");
    }
};

class PrintDirHistoryHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        std::stringstream ss;
        ss << "--- Directory History Entries ---\n";
        vector<std::pair<string, string>> dirs = kvTable.getByPrefix(DIR_HISTORY_ENTRY_PREFIX);
        std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) { 
            return a.first < b.first; 
        });
        
        if (dirs.empty()) {
            ss << "No directory entries found.\n";
        } else {
            for (const auto& pair : dirs) {
                ss << "  " << pair.first << ": " << pair.second << "\n";
            }
        }

        ss << "\n--- TTY Pointers to History Entries ---\n";
        vector<std::pair<string, string>> ptsPointers = 
            kvTable.getByPrefix(DIR_HISTORY_POINTER_PREFIX);
        std::sort(ptsPointers.begin(), ptsPointers.end(), 
            [](const auto &a, const auto &b) { return a.first < b.first; });
        
        if (ptsPointers.empty()) {
            ss << "No TTY pointers found.\n";
        } else {
            for (const auto& pair : ptsPointers) {
                ss << "  " << pair.first << ": " << pair.second << "\n";
            }
        }
        
        ss << "\n--- Last Touched Directory ---\n";
        string lastTouchedIndex = kvTable.get(INDEX_OF_LAST_TOUCHED_DIR_KEY);
        if (!lastTouchedIndex.empty()) {
            string lastTouchedKey = string(DIR_HISTORY_ENTRY_PREFIX) + lastTouchedIndex;
            string lastTouchedValue = kvTable.get(lastTouchedKey);
            ss << "  Index: " << lastTouchedIndex << ", Path: " << lastTouchedValue << "\n";
        } else {
            ss << "No last touched directory recorded.\n";
        }
        
        return CmdResult::success(ss.str());
    }
};

#endif // DATABASE_HANDLER_H

system_handler.h call it systemHandler.h  - System and info commands
#ifndef SYSTEM_HANDLER_H
#define SYSTEM_HANDLER_H

#include "handler_interface.h"
#include "KVTable.h"

extern volatile int running;

class HelpHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        static const string HELP_MESSAGE =
            "Usage: daemon [OPTIONS] <COMMAND>\n\n"
            "Manage terminal history and database entries.\n\n"
            "Commands:\n"
            "  openedTty               Notify daemon that a terminal has been opened.\n"
            "  closedTty               Notify daemon that a terminal has been closed.\n"
            "  updateDirHistory        Update the directory history for a terminal.\n"
            "  cdForward               Move forward in the directory history.\n"
            "  cdBackward              Move backward in the directory history.\n"
            "  showTerminalInstance    Show the current terminal instance.\n"
            "  deleteEntry             Delete a specific entry from the database by key.\n"
            "  deleteEntriesByPrefix   Delete all entries with a specific prefix.\n"
            "  showDB                  Display all entries in the database.\n"
            "  ping                    Ping the daemon and receive pong response.\n"
            "  setKeyboard             Set the keyboard by name and execute restart script.\n"
            "  quit                    Shutdown the daemon.\n\n"
            "Options:\n"
            "  --help                  Display this help message.\n";
        
        return CmdResult::success(HELP_MESSAGE);
    }
};

class PingHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        return CmdResult::success("pong\n");
    }
};

class QuitHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        running = 0;
        return CmdResult::success("Shutting down daemon.\n");
    }
};

class GetKeyboardPathHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        string path = kvTable.get("keyboardPath");
        if (path.empty()) {
            return CmdResult::error("Keyboard path not found\n");
        }
        return CmdResult::success(path + "\n");
    }
};

class GetMousePathHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        string path = kvTable.get("mousePath");
        if (path.empty()) {
            return CmdResult::error("Mouse path not found\n");
        }
        return CmdResult::success(path + "\n");
    }
};

class GetSocketPathHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        return CmdResult::success(string(SOCKET_PATH) + "\n");
    }
};

class GetDirHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string dirName = command[COMMAND_ARG_DIR_NAME].get<string>();
        string result;
        if (dirName == "base") {
            result = directories.base;
        } else if (dirName == "data") {
            result = directories.data;
        } else if (dirName == "mappings") {
            result = directories.mappings;
        } else {
            return CmdResult::error("Unknown directory name: " + dirName + "\n");
        }
        return CmdResult::success(result + "\n");
    }
};

class GetFileHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string fileName = command[COMMAND_ARG_FILE_NAME].get<string>();
        for (const auto& file : files.files) {
            if (file.name.find(fileName) != string::npos) {
                return CmdResult::success(file.fullPath() + "\n");
            }
        }
        return CmdResult::error("File not found: " + fileName + "\n");
    }
};

#endif // SYSTEM_HANDLER_H

keyboard_handler.h call it KeyboardHandler.h 
#ifndef KEYBOARD_HANDLER_H
#define KEYBOARD_HANDLER_H

#include "handler_interface.h"
#include "script_executor.h"

class SetKeyboardHandler : public BaseHandler {
private:
    static string previousKeyboard;
    static bool toggleKeyboardsWhenActiveWindowChanges;
    
    static const vector<string> KNOWN_KEYBOARDS;

public:
    CmdResult handle(const json& command, int client_sock) override;
    
    static void setToggleKeyboards(bool enable) {
        toggleKeyboardsWhenActiveWindowChanges = enable;
    }
};

class ShouldLogHandler : public BaseHandler {
private:
    static bool shouldLog;
    
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
        shouldLog = (enableStr == COMMAND_VALUE_TRUE);
        return CmdResult::success(
            string("Logging ") + (shouldLog ? "enabled" : "disabled") + "\n"
        );
    }
    
    static bool getLoggingEnabled() { return shouldLog; }
};

class GetShouldLogHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)command;
        (void)client_sock;
        return CmdResult::success(
            ShouldLogHandler::getLoggingEnabled() ? 
            COMMAND_VALUE_TRUE : COMMAND_VALUE_FALSE
        );
    }
};

class ToggleKeyboardsWhenActiveWindowChangesHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        string enableStr = command[COMMAND_ARG_ENABLE].get<string>();
        bool enable = (enableStr == COMMAND_VALUE_TRUE);
        SetKeyboardHandler::setToggleKeyboards(enable);
        
        return CmdResult::success(
            string("Return to default keyboard on next window change: ") + 
            (enable ? "no" : "yes") + "\n"
        );
    }
};

class ActiveWindowChangedHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        (void)client_sock;
        if (ShouldLogHandler::getLoggingEnabled()) {
            string logMessage = "[ACTIVE_WINDOW_CHANGED] ";
            logMessage += "windowTitle: " + 
                         command[COMMAND_ARG_WINDOW_TITLE].get<string>() + ", ";
            logMessage += "wmClass: " + 
                         command[COMMAND_ARG_WM_CLASS].get<string>() + ", ";
            logMessage += "wmInstance: " + 
                         command[COMMAND_ARG_WM_INSTANCE].get<string>() + ", ";
            logMessage += "windowId: " + 
                         std::to_string(command[COMMAND_ARG_WINDOW_ID].get<long>()) + "\n";
            
            if (g_logFile.is_open()) {
                g_logFile << logMessage;
                g_logFile.flush();
            }
        }
        return CmdResult::success("Active window info received and logged.\n");
    }
};

#endif // KEYBOARD_HANDLER_H

 command_registry.h call it CommandRegistry.h - Central registration
 #ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include "command_types.h"
#include "terminal_handler.h"
#include "database_handler.h"
#include "system_handler.h"
#include "keyboard_handler.h"
#include <memory>
#include <unordered_map>

class CommandRegistry {
private:
    std::unordered_map<string, std::unique_ptr<ICommandHandler>> handlers;
    std::unordered_map<string, CommandSignature> signatures;
    
    static CommandRegistry* instance;
    
    CommandRegistry() {
        registerAllCommands();
    }
    
    void registerCommand(const string& name, 
                        const vector<string>& requiredArgs,
                        std::unique_ptr<ICommandHandler> handler) {
        signatures.emplace(name, CommandSignature(name, requiredArgs));
        handlers[name] = std::move(handler);
    }
    
    void registerAllCommands() {
        // Help & Info
        registerCommand(COMMAND_EMPTY, {}, 
            std::make_unique<HelpHandler>());
        registerCommand(COMMAND_HELP_DDASH, {}, 
            std::make_unique<HelpHandler>());
        registerCommand(COMMAND_PING, {}, 
            std::make_unique<PingHandler>());
        registerCommand(COMMAND_QUIT, {}, 
            std::make_unique<QuitHandler>());
        
        // Terminal commands
        registerCommand(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY}, 
            std::make_unique<OpenedTtyHandler>());
        registerCommand(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY}, 
            std::make_unique<ClosedTtyHandler>());
        registerCommand(COMMAND_UPDATE_DIR_HISTORY, {COMMAND_ARG_TTY, COMMAND_ARG_PWD}, 
            std::make_unique<UpdateDirHistoryHandler>());
        registerCommand(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY}, 
            std::make_unique<CdForwardHandler>());
        registerCommand(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY}, 
            std::make_unique<CdBackwardHandler>());
        registerCommand(COMMAND_SHOW_TERMINAL_INSTANCE, {COMMAND_ARG_TTY}, 
            std::make_unique<ShowTerminalInstanceHandler>());
        registerCommand(COMMAND_SHOW_ALL_TERMINAL_INSTANCES, {}, 
            std::make_unique<ShowAllTerminalInstancesHandler>());
        
        // Database commands
        registerCommand(COMMAND_SHOW_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}, 
            std::make_unique<ShowEntriesByPrefixHandler>());
        registerCommand(COMMAND_DELETE_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX}, 
            std::make_unique<DeleteEntriesByPrefixHandler>());
        registerCommand(COMMAND_SHOW_DB, {}, 
            std::make_unique<ShowDbHandler>());
        registerCommand(COMMAND_DELETE_ENTRY, {COMMAND_ARG_KEY}, 
            std::make_unique<DeleteEntryHandler>());
        registerCommand(COMMAND_PRINT_DIR_HISTORY, {}, 
            std::make_unique<PrintDirHistoryHandler>());
        registerCommand(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}, 
            std::make_unique<UpsertEntryHandler>());
        registerCommand(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}, 
            std::make_unique<GetEntryHandler>());
        
        // System commands
        registerCommand(COMMAND_GET_KEYBOARD_PATH, {}, 
            std::make_unique<GetKeyboardPathHandler>());
        registerCommand(COMMAND_GET_MOUSE_PATH, {}, 
            std::make_unique<GetMousePathHandler>());
        registerCommand(COMMAND_GET_SOCKET_PATH, {}, 
            std::make_unique<GetSocketPathHandler>());
        registerCommand(COMMAND_GET_DIR, {COMMAND_ARG_DIR_NAME}, 
            std::make_unique<GetDirHandler>());
        registerCommand(COMMAND_GET_FILE, {COMMAND_ARG_FILE_NAME}, 
            std::make_unique<GetFileHandler>());
        
        // Keyboard/logging commands
        registerCommand(COMMAND_SET_KEYBOARD, {COMMAND_ARG_KEYBOARD_NAME}, 
            std::make_unique<SetKeyboardHandler>());
        registerCommand(COMMAND_SHOULD_LOG, {COMMAND_ARG_ENABLE}, 
            std::make_unique<ShouldLogHandler>());
        registerCommand(COMMAND_GET_SHOULD_LOG, {}, 
            std::make_unique<GetShouldLogHandler>());
        registerCommand(COMMAND_TOGGLE_KEYBOARDS_WHEN_ACTIVE_WINDOW_CHANGES, 
            {COMMAND_ARG_ENABLE}, 
            std::make_unique<ToggleKeyboardsWhenActiveWindowChangesHandler>());
        registerCommand(COMMAND_ACTIVE_WINDOW_CHANGED, 
            {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS, 
             COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID}, 
            std::make_unique<ActiveWindowChangedHandler>());
    }

public:
    static CommandRegistry& getInstance() {
        if (!instance) {
            instance = new CommandRegistry();
        }
        return *instance;
    }
    
    ICommandHandler* getHandler(const string& commandName) {
        auto it = handlers.find(commandName);
        return (it != handlers.end()) ? it->second.get() : nullptr;
    }
    
    const CommandSignature* getSignature(const string& commandName) {
        auto it = signatures.find(commandName);
        return (it != signatures.end()) ? &it->second : nullptr;
    }
    
    vector<string> getAllCommandNames() const {
        vector<string> names;
        for (const auto& pair : signatures) {
            names.push_back(pair.first);
        }
        return names;
    }
};

#endif // COMMAND_REGISTRY_H

command_dispatcher.h call it commandDispatcher.h - New simplified mainCommand
#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include "command_registry.h"
#include "command_validator.h"

class CommandDispatcher {
public:
    static CmdResult dispatch(const json& command, int client_sock) {
        try {
            // Validate command structure
            CmdResult validation = CommandValidator::validate(command);
            if (validation.status != 0) {
                return validation;
            }
            
            // Get command name
            string commandName = command[COMMAND_KEY].get<string>();
            
            // Get handler from registry
            CommandRegistry& registry = CommandRegistry::getInstance();
            ICommandHandler* handler = registry.getHandler(commandName);
            
            if (!handler) {
                return CmdResult::error("Unknown command: " + commandName + "\n");
            }
            
            // Execute handler
            CmdResult result = handler->handle(command, client_sock);
            
            // Ensure message ends with newline
            if (!result.message.empty() && result.message.back() != '\n') {
                result.message += "\n";
            }
            
            return result;
            
        } catch (const std::exception& e) {
            return CmdResult::error(
                string("Command execution error: ") + e.what() + "\n"
            );
        }
    }
};

// Main entry point (replaces old mainCommand)
inline int mainCommand(const json& command, int client_sock) {
    CmdResult result = CommandDispatcher::dispatch(command, client_sock);
    
    // Don't write response for closedTty (special case)
    if (command.contains(COMMAND_KEY) && 
        command[COMMAND_KEY] == COMMAND_CLOSED_TTY) {
        return 0;
    }
    
    // Write response to client
    write(client_sock, result.message.c_str(), result.message.length());
    return 0;
}

#endif // COMMAND_DISPATCHER_H

command_validator.h call it commandValidator.h - Command validation
#ifndef COMMAND_VALIDATOR_H
#define COMMAND_VALIDATOR_H

#include "command_registry.h"

class CommandValidator {
public:
    static CmdResult validate(const json& command) {
        // Check if command key exists
        if (!command.contains(COMMAND_KEY)) {
            return CmdResult::error("Missing command key\n");
        }
        
        string commandName = command[COMMAND_KEY].get<string>();
        
        // Get command signature
        CommandRegistry& registry = CommandRegistry::getInstance();
        const CommandSignature* signature = registry.getSignature(commandName);
        
        if (!signature) {
            return CmdResult::error("Unknown command: " + commandName + "\n");
        }
        
        // Validate required arguments
        for (const string& arg : signature->requiredArgs) {
            if (!command.contains(arg)) {
                return CmdResult::error(
                    "Missing required argument: " + arg + "\n"
                );
            }
        }
        
        return CmdResult::success();
    }
};

#endif // COMMAND_VALIDATOR_H

// ============================================================================
// Summary of the split architecture:
// ============================================================================
// 
// OLD: mainCommand.cpp (1000+ lines, everything mixed together)
// 
// NEW:
// ├── command_types.h              (50 lines) - Data structures
// ├── handler_interface.h          (40 lines) - Base classes
// ├── handlers/
// │   ├── terminal_handler.h      (80 lines) - Terminal commands
// │   ├── database_handler.h      (150 lines) - DB commands
// │   ├── system_handler.h        (100 lines) - System/info commands
// │   └── keyboard_handler.h      (120 lines) - Keyboard/logging
// ├── command_registry.h          (150 lines) - Registration
// ├── command_validator.h         (40 lines) - Validation
// └── command_dispatcher.h        (50 lines) - Main dispatch
//
// Total: ~780 lines split across 9 focused files
//
// Benefits:
// 1. Each handler is independently testable
// 2. Adding new commands is simple (create handler, register)
// 3. Clear separation of concerns
// 4. Easy to find and modify specific functionality
//
// but we need to put the .h files in include/ and the .cpp files in src/ instead
// ============================================================================

#endif // COMMAND_VALIDATOR_H


How to Add a New Command
cpp// 1. Create handler in appropriate file
class MyNewHandler : public BaseHandler {
public:
    CmdResult handle(const json& command, int client_sock) override {
        // Your logic here
        return CmdResult::success("Done!\n");
    }
};

// 2. Register in command_registry.h
registerCommand("myNewCommand", {"arg1", "arg2"}, 
    std::make_unique<MyNewHandler>());

// Done! The dispatcher handles everything else