#include "mainCommand.h"
#include "Constants.h"
#include "Utils.h"
#include <string>
#include <unistd.h>

// Include all command handler headers
#include "cmdBrowser.h"
#include "cmdDatabase.h"
#include "cmdInput.h"
#include "cmdLogging.h"
#include "cmdLoom.h"
#include "cmdPeer.h"
#include "cmdPort.h"
#include "cmdSystem.h"
#include "cmdTerminal.h"
#include "cmdWindow.h"
#include "cmdWireGuard.h"

using namespace std;

// Global state - shared across command handlers
int g_clientSocket = -1;
unsigned int shouldLog = LOG_ALL;
bool g_keyboardEnabled = false;

// Command registry - defines all available commands and their signatures
const CommandSignature COMMAND_REGISTRY[] = {
    // Help Commands
    CommandSignature(COMMAND_EMPTY, {}, "Show help message"),
    CommandSignature(COMMAND_HELP, {}, "Show help message"),
    CommandSignature(COMMAND_HELP_DDASH, {}, "Show help message"),

    // Terminal History Commands
    CommandSignature(COMMAND_OPENED_TTY, {COMMAND_ARG_TTY},
                     "Notify daemon that a terminal was opened"),
    CommandSignature(COMMAND_CLOSED_TTY, {COMMAND_ARG_TTY},
                     "Notify daemon that a terminal was closed"),
    CommandSignature(COMMAND_UPDATE_DIR_HISTORY, {COMMAND_ARG_TTY, COMMAND_ARG_PWD},
                     "Update directory history for a terminal"),
    CommandSignature(COMMAND_CD_FORWARD, {COMMAND_ARG_TTY},
                     "Navigate forward in directory history (Ctrl+Down)"),
    CommandSignature(COMMAND_CD_BACKWARD, {COMMAND_ARG_TTY},
                     "Navigate backward in directory history (Ctrl+Up)"),
    CommandSignature(COMMAND_SHELL_SIGNAL, {COMMAND_ARG_SIGNAL},
                     "Handle shell signal events"),
    CommandSignature(COMMAND_SHOW_TERMINAL_INSTANCE, {COMMAND_ARG_TTY},
                     "Show terminal instance info for a TTY"),
    CommandSignature(COMMAND_SHOW_ALL_TERMINAL_INSTANCES, {},
                     "Show all active terminal instances"),
    CommandSignature(COMMAND_PRINT_DIR_HISTORY, {},
                     "Print all directory history entries"),
    CommandSignature(COMMAND_EMPTY_DIR_HISTORY_TABLE, {},
                     "Clear all directory history entries"),

    // Database Commands
    CommandSignature(COMMAND_DELETE_ENTRY, {COMMAND_ARG_KEY},
                     "Delete a setting entry by key"),
    CommandSignature(COMMAND_SHOW_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX},
                     "(Deprecated) Show entries by prefix"),
    CommandSignature(COMMAND_DELETE_ENTRIES_BY_PREFIX, {COMMAND_ARG_PREFIX},
                     "(Deprecated) Delete entries by prefix"),
    CommandSignature(COMMAND_SHOW_DB, {},
                     "Show database summary (terminal history, devices, settings)"),
    CommandSignature(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE},
                     "Insert or update a setting entry"),
    CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY},
                     "Get a setting entry by key"),

    // System Commands
    CommandSignature(COMMAND_PING, {}, "Ping the daemon (returns 'pong')"),
    CommandSignature(COMMAND_QUIT, {}, "Stop the daemon"),
    CommandSignature(COMMAND_GET_DIR, {COMMAND_ARG_DIR_NAME},
                     "Get daemon directory path (base, data, mappings)"),
    CommandSignature(COMMAND_GET_FILE, {COMMAND_ARG_FILE_NAME},
                     "Get file path from data or mappings directory"),
    CommandSignature(COMMAND_LIST_COMMANDS, {},
                     "List all available command names"),

    // Input Device Commands
    CommandSignature(COMMAND_GET_KEYBOARD_PATH, {},
                     "Get path to the grabbed keyboard device"),
    CommandSignature(COMMAND_GET_MOUSE_PATH, {},
                     "Get path to the grabbed mouse device"),
    CommandSignature(COMMAND_GET_SOCKET_PATH, {},
                     "Get the daemon's UNIX socket path"),
    CommandSignature(COMMAND_SET_KEYBOARD,
                     {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS,
                      COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID},
                     "Simulate active window change (for debugging)"),
    CommandSignature(COMMAND_GET_KEYBOARD, {},
                     "Get current keyboard mapping state"),
    CommandSignature(COMMAND_GET_KEYBOARD_ENABLED, {},
                     "Check if keyboard input is enabled"),
    CommandSignature(COMMAND_TOGGLE_KEYBOARD, {COMMAND_ARG_ENABLE},
                     "Toggle auto-keyboard switching on window change"),
    CommandSignature(COMMAND_DISABLE_KEYBOARD, {},
                     "Disable keyboard input grabbing"),
    CommandSignature(COMMAND_ENABLE_KEYBOARD, {},
                     "Enable keyboard input grabbing"),
    CommandSignature(COMMAND_SIMULATE_INPUT, {},
                     "Simulate input event or type text",
                     "--type --code --value (raw event) OR --string (text)"),

    // Logging Commands
    CommandSignature(COMMAND_SHOULD_LOG, {COMMAND_ARG_ENABLE},
                     "Enable or disable logging"),
    CommandSignature(COMMAND_GET_SHOULD_LOG, {},
                     "Get current logging state"),
    CommandSignature(COMMAND_REGISTER_LOG_LISTENER, {},
                     "Register as a live log listener (streaming)"),
    CommandSignature(COMMAND_ADD_LOG_FILTER, {COMMAND_ARG_ACTION},
                     "Add granular input event log filter",
                     "--type --code --value --devicePathRegex --isKeyboard"),
    CommandSignature(COMMAND_REMOVE_LOG_FILTER, {},
                     "Remove a log filter",
                     "--type --code --value --devicePathRegex --isKeyboard"),
    CommandSignature(COMMAND_LIST_LOG_FILTERS, {},
                     "List all active log filters"),
    CommandSignature(COMMAND_CLEAR_LOG_FILTERS, {},
                     "Clear all log filters"),

    // Window/Context Commands
    CommandSignature(COMMAND_ACTIVE_WINDOW_CHANGED,
                     {COMMAND_ARG_WINDOW_TITLE, COMMAND_ARG_WM_CLASS,
                      COMMAND_ARG_WM_INSTANCE, COMMAND_ARG_WINDOW_ID},
                     "Notify daemon of active window change (from GNOME ext)"),
    CommandSignature(COMMAND_SET_ACTIVE_TAB_URL, {COMMAND_ARG_URL},
                     "Set active browser tab URL (from Chrome ext)"),
    CommandSignature(COMMAND_REGISTER_NATIVE_HOST, {},
                     "Register Chrome native messaging host"),
    CommandSignature(COMMAND_FOCUS_CHATGPT, {},
                     "Request focus on ChatGPT browser tab"),
    CommandSignature(COMMAND_FOCUS_ACK, {},
                     "Acknowledge focus request"),
    CommandSignature(COMMAND_GET_ACTIVE_CONTEXT, {},
                     "Get current active window context (JSON)"),
    CommandSignature(COMMAND_REGISTER_WINDOW_EXTENSION, {},
                     "Register GNOME window tracking extension"),
    CommandSignature(COMMAND_LIST_WINDOWS, {},
                     "List all tracked windows"),
    CommandSignature(COMMAND_ACTIVATE_WINDOW, {COMMAND_ARG_WINDOW_ID},
                     "Activate a window by ID"),

    // Macro Commands
    CommandSignature(COMMAND_GET_MACROS, {},
                     "Get all configured macros (JSON)"),
    CommandSignature(COMMAND_UPDATE_MACROS, {COMMAND_ARG_VALUE},
                     "Update macro configuration"),
    CommandSignature(COMMAND_GET_EVENT_FILTERS, {},
                     "Get event filters for macro system"),
    CommandSignature(COMMAND_SET_EVENT_FILTERS, {COMMAND_ARG_VALUE},
                     "Set event filters for macro system"),

    // Port Management Commands
    CommandSignature(COMMAND_GET_PORT, {COMMAND_ARG_KEY},
                     "Get assigned port for an app/service"),
    CommandSignature(COMMAND_SET_PORT, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE},
                     "Assign a port to an app/service"),
    CommandSignature(COMMAND_LIST_PORTS, {},
                     "List all port assignments"),
    CommandSignature(COMMAND_DELETE_PORT, {COMMAND_ARG_KEY},
                     "Delete a port assignment"),

    // Loom Commands
    CommandSignature(COMMAND_IS_LOOM_ACTIVE, {},
                     "Check if Loom screen streaming is active"),
    CommandSignature(COMMAND_RESTART_LOOM, {},
                     "Start/restart Loom streaming server and client"),
    CommandSignature(COMMAND_STOP_LOOM, {},
                     "Stop Loom streaming"),
    CommandSignature(COMMAND_GENERATE_LOOM_TOKEN, {},
                     "Generate a new Loom authentication token"),
    CommandSignature(COMMAND_REVOKE_LOOM_TOKENS, {},
                     "Revoke all Loom authentication tokens"),
    CommandSignature(COMMAND_RESET_CLOCK, {},
                     "Reset Loom frame clock"),

    // Public Transportation Commands
    CommandSignature(COMMAND_PUBLIC_TRANSPORTATION_START_PROXY, {},
                     "Start public transportation proxy server"),
    CommandSignature(COMMAND_PUBLIC_TRANSPORTATION_OPEN_APP, {},
                     "Open public transportation app"),

    // Test/Debug Commands
    CommandSignature(COMMAND_TEST_INTEGRITY, {},
                     "Run internal integrity tests"),
    CommandSignature(COMMAND_TEST_LSOF, {COMMAND_ARG_PORT},
                     "Test lsof on a port"),
    CommandSignature(COMMAND_TEST_ECHO, {COMMAND_ARG_MESSAGE},
                     "Echo a test message"),
    CommandSignature(COMMAND_TEST_LSOF_SCRIPT, {COMMAND_ARG_PORT},
                     "Test lsof script on a port"),

    // Peer Networking Commands
    CommandSignature(COMMAND_SET_PEER_CONFIG, {},
                     "Configure peer networking role and identity",
                     "--role (leader|worker) --id <peer_id> [--leader <ip>]"),
    CommandSignature(COMMAND_GET_PEER_STATUS, {},
                     "Show current peer configuration and connection status"),
    CommandSignature(COMMAND_REGISTER_PEER, {},
                     "(Internal) Register a peer connection"),
    CommandSignature(COMMAND_LIST_PEERS, {},
                     "List all registered peers in the network"),
    CommandSignature(COMMAND_GET_PEER_INFO, {COMMAND_ARG_PEER},
                     "Get detailed info about a specific peer"),
    CommandSignature(COMMAND_EXEC_ON_PEER, {COMMAND_ARG_PEER, COMMAND_ARG_DIRECTORY, COMMAND_ARG_SHELL_CMD},
                     "Execute a command on a remote peer in specified directory"),
    CommandSignature(COMMAND_EXEC_REQUEST, {COMMAND_ARG_DIRECTORY, COMMAND_ARG_SHELL_CMD},
                     "(Internal) Handle exec request from another peer"),

    // WireGuard Setup Commands
    CommandSignature(COMMAND_SETUP_WIREGUARD_PEER, {COMMAND_ARG_NAME},
                     "Set up WireGuard on a peer and register with daemon",
                     "--host <ip> --vpnIp <ip> --mac <addr> --dualBoot --privateKey <key>"),
    CommandSignature(COMMAND_LIST_WIREGUARD_PEERS, {},
                     "List peers configured in WireGuard on the VPS"),
    CommandSignature(COMMAND_GET_WIREGUARD_IP, {},
                     "Get the local WireGuard (wg0) interface IP address"),
};

const size_t COMMAND_REGISTRY_SIZE =
    sizeof(COMMAND_REGISTRY) / sizeof(COMMAND_REGISTRY[0]);

// Command handler function pointer type
typedef CmdResult (*CommandHandler)(const json &);

// Command dispatch entry
struct CommandDispatch {
  const char *name;
  CommandHandler handler;
};

// Command dispatch table - maps command names to handler functions
static const CommandDispatch COMMAND_HANDLERS[] = {
    // System commands
    {COMMAND_EMPTY, handleHelp},
    {COMMAND_HELP, handleHelp},
    {COMMAND_HELP_DDASH, handleHelp},
    {COMMAND_PING, handlePing},
    {COMMAND_QUIT, handleQuit},
    {COMMAND_GET_DIR, handleGetDir},
    {COMMAND_GET_FILE, handleGetFile},
    {COMMAND_GET_SOCKET_PATH, handleGetSocketPath},
    {COMMAND_LIST_COMMANDS, handleListCommands},
    {COMMAND_TEST_LSOF, handleTestLsof},
    {COMMAND_TEST_ECHO, handleTestEcho},
    {COMMAND_TEST_LSOF_SCRIPT, handleTestLsofScript},

    // Terminal commands
    {COMMAND_OPENED_TTY, handleOpenedTty},
    {COMMAND_CLOSED_TTY, handleClosedTty},
    {COMMAND_UPDATE_DIR_HISTORY, handleUpdateDirHistory},
    {COMMAND_CD_FORWARD, handleCdForward},
    {COMMAND_CD_BACKWARD, handleCdBackward},
    {COMMAND_SHELL_SIGNAL, handleShellSignal},
    {COMMAND_SHOW_TERMINAL_INSTANCE, handleShowTerminalInstance},
    {COMMAND_SHOW_ALL_TERMINAL_INSTANCES, handleShowAllTerminalInstances},
    {COMMAND_PRINT_DIR_HISTORY, handlePrintDirHistory},
    {COMMAND_EMPTY_DIR_HISTORY_TABLE, handleEmptyDirHistoryTable},

    // Database commands
    {COMMAND_SHOW_ENTRIES_BY_PREFIX, handleShowEntriesByPrefix},
    {COMMAND_DELETE_ENTRIES_BY_PREFIX, handleDeleteEntriesByPrefix},
    {COMMAND_SHOW_DB, handleShowDb},
    {COMMAND_DELETE_ENTRY, handleDeleteEntry},
    {COMMAND_UPSERT_ENTRY, handleUpsertEntry},
    {COMMAND_GET_ENTRY, handleGetEntry},

    // Input commands
    {COMMAND_GET_KEYBOARD_PATH, handleGetKeyboardPath},
    {COMMAND_GET_MOUSE_PATH, handleGetMousePath},
    {COMMAND_SET_KEYBOARD, handleSetKeyboard},
    {COMMAND_GET_KEYBOARD, handleGetKeyboard},
    {COMMAND_GET_KEYBOARD_ENABLED, handleGetKeyboard},
    {COMMAND_TOGGLE_KEYBOARD, handleToggleKeyboard},
    {COMMAND_DISABLE_KEYBOARD, handleDisableKeyboard},
    {COMMAND_ENABLE_KEYBOARD, handleEnableKeyboard},
    {COMMAND_SIMULATE_INPUT, handleSimulateInput},
    {COMMAND_TEST_INTEGRITY, handleTestIntegrity},
    {COMMAND_GET_MACROS, handleGetMacros},
    {COMMAND_UPDATE_MACROS, handleUpdateMacros},

    // Logging commands
    {COMMAND_SHOULD_LOG, handleShouldLog},
    {COMMAND_GET_SHOULD_LOG, handleGetShouldLog},
    {COMMAND_REGISTER_LOG_LISTENER, handleRegisterLogListener},
    {COMMAND_GET_EVENT_FILTERS, handleGetEventFilters},
    {COMMAND_SET_EVENT_FILTERS, handleSetEventFilters},

    // Window commands
    {COMMAND_REGISTER_WINDOW_EXTENSION, handleRegisterWindowExtension},
    {COMMAND_LIST_WINDOWS, handleListWindows},
    {COMMAND_ACTIVATE_WINDOW, handleActivateWindow},
    {COMMAND_ACTIVE_WINDOW_CHANGED, handleActiveWindowChanged},
    {COMMAND_GET_ACTIVE_CONTEXT, handleGetActiveContext},

    // Browser commands
    {COMMAND_SET_ACTIVE_TAB_URL, handleSetActiveTabUrl},
    {COMMAND_REGISTER_NATIVE_HOST, handleRegisterNativeHost},
    {COMMAND_FOCUS_CHATGPT, handleFocusChatGPT},
    {COMMAND_FOCUS_ACK, handleFocusAck},

    // Port commands
    {COMMAND_GET_PORT, handleGetPort},
    {COMMAND_SET_PORT, handleSetPort},
    {COMMAND_LIST_PORTS, handleListPorts},
    {COMMAND_DELETE_PORT, handleDeletePort},
    {COMMAND_PUBLIC_TRANSPORTATION_START_PROXY, handlePublicTransportationStartProxy},
    {COMMAND_PUBLIC_TRANSPORTATION_OPEN_APP, handlePublicTransportationOpenApp},

    // Loom commands
    {COMMAND_RESET_CLOCK, handleResetClock},
    {COMMAND_IS_LOOM_ACTIVE, handleIsLoomActive},
    {COMMAND_RESTART_LOOM, handleRestartLoom},
    {COMMAND_STOP_LOOM, handleStopLoom},
    {COMMAND_GENERATE_LOOM_TOKEN, handleGenerateLoomToken},
    {COMMAND_REVOKE_LOOM_TOKENS, handleRevokeLoomTokens},

    // Peer commands
    {COMMAND_SET_PEER_CONFIG, handleSetPeerConfig},
    {COMMAND_GET_PEER_STATUS, handleGetPeerStatus},
    {COMMAND_REGISTER_PEER, handleRegisterPeer},
    {COMMAND_LIST_PEERS, handleListPeers},
    {COMMAND_GET_PEER_INFO, handleGetPeerInfo},
    {COMMAND_EXEC_ON_PEER, handleExecOnPeer},
    {COMMAND_EXEC_REQUEST, handleExecRequest},

    // WireGuard commands
    {COMMAND_SETUP_WIREGUARD_PEER, handleSetupWireGuardPeer},
    {COMMAND_LIST_WIREGUARD_PEERS, handleListWireGuardPeers},
    {COMMAND_GET_WIREGUARD_IP, handleGetWireGuardIp},
};

static const size_t COMMAND_HANDLERS_SIZE =
    sizeof(COMMAND_HANDLERS) / sizeof(COMMAND_HANDLERS[0]);

// Get log context for a command
static std::pair<unsigned int, std::string>
getCommandLogContext(const std::string &commandName) {
  if (commandName == COMMAND_FOCUS_CHATGPT ||
      commandName == COMMAND_SET_ACTIVE_TAB_URL ||
      commandName == COMMAND_FOCUS_ACK ||
      commandName == COMMAND_REGISTER_NATIVE_HOST) {
    return {LOG_CHROME, "[Chrome]"};
  }
  if (commandName == COMMAND_UPDATE_DIR_HISTORY ||
      commandName == COMMAND_OPENED_TTY || commandName == COMMAND_CLOSED_TTY ||
      commandName == COMMAND_CD_FORWARD || commandName == COMMAND_CD_BACKWARD ||
      commandName == COMMAND_SHOW_TERMINAL_INSTANCE ||
      commandName == COMMAND_SHOW_ALL_TERMINAL_INSTANCES) {
    return {LOG_TERMINAL, "[Terminal]"};
  }
  if (commandName == COMMAND_ACTIVE_WINDOW_CHANGED) {
    return {LOG_WINDOW, "[Window]"};
  }
  return {LOG_NETWORK, "[Network]"};
}

// Validate command has required arguments
CmdResult validateCommand(const json &command) {
  if (!command.contains(COMMAND_KEY)) {
    return CmdResult(1, "Missing command key");
  }
  string commandName = command[COMMAND_KEY].get<string>();
  const CommandSignature *foundCommand = nullptr;
  for (size_t i = 0; i < COMMAND_REGISTRY_SIZE; ++i) {
    if (COMMAND_REGISTRY[i].name == commandName) {
      foundCommand = &COMMAND_REGISTRY[i];
      break;
    }
  }
  if (!foundCommand) {
    return CmdResult(1, string("Unknown command: ") + commandName +
                            mustEndWithNewLine);
  }
  for (const string &arg : foundCommand->requiredArgs) {
    if (!command.contains(arg)) {
      return CmdResult(1, string("Missing required arg: ") + arg +
                              mustEndWithNewLine);
    }
  }
  return CmdResult(0, "");
}

// Main command dispatcher
int mainCommand(const json &command, int client_sock) {
  g_clientSocket = client_sock;
  string commandStr = command.dump();
  string commandName =
      command.contains(COMMAND_KEY) ? command[COMMAND_KEY].get<string>() : "";
  auto logCtx = getCommandLogContext(commandName);
  logToFile(logCtx.second + " Received command: " + commandStr, logCtx.first);
  CmdResult result;
  try {
    CmdResult integrityCheck = validateCommand(command);
    if (integrityCheck.status != 0) {
      result = integrityCheck;
    } else {
      string commandName = command[COMMAND_KEY].get<string>();
      CommandHandler handler = nullptr;
      for (size_t i = 0; i < COMMAND_HANDLERS_SIZE; ++i) {
        if (COMMAND_HANDLERS[i].name == commandName) {
          handler = COMMAND_HANDLERS[i].handler;
          break;
        }
      }
      if (handler) {
        result = handler(command);
      } else {
        result.status = 1;
        result.message =
            string("Unhandled command: ") + commandStr + mustEndWithNewLine;
      }
    }
  } catch (const std::exception &e) {
    result.status = 1;
    result.message = std::string("error: ") + e.what() + "\n";
  }
  if (!result.message.empty() && result.message.back() != '\n') {
    result.message += "\n";
  }
  write(client_sock, result.message.c_str(), result.message.length());

  if (command[COMMAND_KEY] == "closedTty") {
    return 1;
  }

  // Return 1 (close) for regular commands, 0 (keep) for log listeners.
  if (commandName == COMMAND_REGISTER_LOG_LISTENER ||
      commandName == COMMAND_REGISTER_WINDOW_EXTENSION ||
      commandName == COMMAND_REGISTER_NATIVE_HOST) {
    return 0;
  }
  return 1;
}
