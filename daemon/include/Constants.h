#ifndef CONSTANTS_H
#define CONSTANTS_H

#define OUTPUT_FILE "/opt/automateLinux/daemon/output.txt"
#define SOCKET_PATH "/run/automatelinux/automatelinux-daemon.sock"
#define DIR_HISTORY_POINTER_PREFIX "pointerDevPts"
#define INDEX_OF_LAST_TOUCHED_DIR_KEY "indexOfLastTouchedDir"
#define TTY_KEY "tty"
#define PWD_KEY "pwd"
#define DIR_HISTORY_ENTRY_PREFIX "dirHistory"
#define DIR_HISTORY_DEFAULT_DIR "/opt/automateLinux/"
#define mustEndWithNewLine "\n"
#define COMMAND_KEY "command"
#define COMMAND_EMPTY ""
#define COMMAND_HELP "help"
#define COMMAND_HELP_DDASH "--help"
#define COMMAND_OPENED_TTY "openedTty"
#define COMMAND_CLOSED_TTY "closedTty"
#define COMMAND_UPDATE_DIR_HISTORY "updateDirHistory"
#define COMMAND_CD_FORWARD "cdForward"
#define COMMAND_CD_BACKWARD "cdBackward"
#define COMMAND_SHELL_SIGNAL "shellSignal"
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
#define COMMAND_ARG_SIGNAL "signal"
#define COMMAND_PING "ping"
#define COMMAND_QUIT "quit"
#define COMMAND_GET_PORT "getPort"
#define COMMAND_SET_PORT "setPort"
#define COMMAND_GET_KEYBOARD_PATH "getKeyboardPath"
#define COMMAND_GET_MOUSE_PATH "getMousePath"
#define COMMAND_GET_SOCKET_PATH "getSocketPath"
#define COMMAND_SET_KEYBOARD "setKeyboard"
#define COMMAND_GET_KEYBOARD "getKeyboard"
#define COMMAND_GET_KEYBOARD_ENABLED "getKeyboardEnabled"
#define COMMAND_ARG_KEYBOARD_NAME "keyboardName"
#define PREFIX_KEYBOARD "corsairKeyBoardLogiMouse"
#define ALL_KEYBOARD "All"
#define CODE_KEYBOARD "code"
#define GNOME_TERMINAL_KEYBOARD "gnome-terminal-server"
#define GOOGLE_CHROME_KEYBOARD "google-chrome"
#define DEFAULT_KEYBOARD "DefaultKeyboard"
#define TEST_KEYBOARD "TestKeyboard"
#define KEYBOARD_DISCOVERY_CMD                                                 \
  "ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd'"
#define KEYBOARD_INPUT_PATH "/dev/input/by-id/"
#define MOUSE_DISCOVERY_CMD                                                    \
  "awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, "   \
  "/event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices"
#define MOUSE_INPUT_PATH "/dev/input/"
#define KEYBOARD_PATH_KEY "keyboardPath"
#define MOUSE_PATH_KEY "mousePath"
#define COMMAND_VALUE_TRUE "true"
#define COMMAND_VALUE_FALSE "false"
#define COMMAND_SHOULD_LOG "shouldLog"
#define COMMAND_GET_SHOULD_LOG "getShouldLog"
#define COMMAND_TOGGLE_KEYBOARD "toggleKeyboard"
#define COMMAND_ARG_ENABLE "enable"
#define COMMAND_GET_DIR "getDir"
#define COMMAND_ARG_DIR_NAME "dirName"
#define COMMAND_GET_FILE "getFile"
#define COMMAND_ARG_FILE_NAME "fileName"
#define COMMAND_ACTIVE_WINDOW_CHANGED "activeWindowChanged"
#define COMMAND_SET_ACTIVE_TAB_URL "setActiveTabUrl"
#define COMMAND_REGISTER_NATIVE_HOST "registerNativeHost"
#define COMMAND_FOCUS_ACK "focusAck"
#define COMMAND_FOCUS_CHATGPT "focusChatGPT"
#define COMMAND_GET_MACROS "getMacros"
#define COMMAND_UPDATE_MACROS "updateMacros"
#define COMMAND_GET_ACTIVE_CONTEXT "getActiveContext"
#define COMMAND_GET_EVENT_FILTERS "getEventFilters"
#define COMMAND_SET_EVENT_FILTERS "setEventFilters"
#define COMMAND_REGISTER_LOG_LISTENER "registerLogListener"
#define COMMAND_DISABLE_KEYBOARD "disableKeyboard"
#define COMMAND_ENABLE_KEYBOARD "enableKeyboard"
#define COMMAND_TEST_INTEGRITY "testIntegrity"
#define COMMAND_SIMULATE_INPUT "simulateInput"
#define COMMAND_ADD_LOG_FILTER "addLogFilter"
#define COMMAND_REMOVE_LOG_FILTER "removeLogFilter"
#define COMMAND_LIST_LOG_FILTERS "listLogFilters"
#define COMMAND_CLEAR_LOG_FILTERS "clearLogFilters"
#define COMMAND_EMPTY_DIR_HISTORY_TABLE "emptyDirHistoryTable"
#define COMMAND_REGISTER_WINDOW_EXTENSION "registerWindowExtension"
#define COMMAND_LIST_WINDOWS "listWindows"
#define COMMAND_ACTIVATE_WINDOW "activateWindow"
#define COMMAND_RESET_CLOCK "resetClock"
#define COMMAND_IS_LOOM_ACTIVE "isLoomActive"
#define COMMAND_RESTART_LOOM "restartLoom"
#define COMMAND_STOP_LOOM "stopLoom"
#define COMMAND_PUBLIC_TRANSPORTATION_START_PROXY                              \
  "publicTransportationStartProxy"
#define COMMAND_PUBLIC_TRANSPORTATION_OPEN_APP "publicTransportationOpenApp"
#define COMMAND_LIST_PORTS "listPorts"
#define COMMAND_GENERATE_LOOM_TOKEN "generateLoomToken"
#define COMMAND_REVOKE_LOOM_TOKENS "revokeLoomTokens"

// App Management Commands
#define COMMAND_START_APP "startApp"
#define COMMAND_STOP_APP "stopApp"
#define COMMAND_RESTART_APP "restartApp"
#define COMMAND_APP_STATUS "appStatus"
#define COMMAND_LIST_APPS "listApps"
#define COMMAND_BUILD_APP "buildApp"
#define COMMAND_INSTALL_APP_DEPS "installAppDeps"

// App Arguments
#define COMMAND_ARG_APP "app"
#define COMMAND_ARG_MODE "mode"

#define COMMAND_DELETE_PORT "deletePort"
#define COMMAND_TEST_LSOF "testLsof"
#define COMMAND_TEST_ECHO "testEcho"
#define COMMAND_WRITE_TEST_LOG "writeTestLog"
#define COMMAND_TEST_LSOF_SCRIPT "testLsofScript"

#define COMMAND_LIST_COMMANDS "listCommands"

// Peer Networking Commands
#define COMMAND_SET_PEER_CONFIG "setPeerConfig"
#define COMMAND_GET_PEER_STATUS "getPeerStatus"
#define COMMAND_REGISTER_PEER "registerPeer"
#define COMMAND_LIST_PEERS "listPeers"
#define COMMAND_GET_PEER_INFO "getPeerInfo"
#define COMMAND_EXEC_ON_PEER "execOnPeer"
#define COMMAND_EXEC_REQUEST "execRequest"
#define COMMAND_REMOTE_PULL "remotePull"
#define COMMAND_REMOTE_BD "remoteBd"
#define COMMAND_REMOTE_DEPLOY_DAEMON "remoteDeployDaemon"
#define COMMAND_DB_SANITY_CHECK "dbSanityCheck"
#define COMMAND_REGISTER_WORKER "registerWorker"

// Peer Arguments
#define COMMAND_ARG_ROLE "role"
#define COMMAND_ARG_LEADER "leader"
#define COMMAND_ARG_ID "id"
#define COMMAND_ARG_PEER "peer"
#define COMMAND_ARG_DIRECTORY "directory"
#define COMMAND_ARG_SHELL_CMD "shellCmd"

// Peer Roles
#define PEER_ROLE_LEADER "leader"
#define PEER_ROLE_WORKER "worker"

// Peer Networking Constants
#define PEER_TCP_PORT 3600

// WireGuard Setup Commands
#define COMMAND_SETUP_WIREGUARD_PEER "setupWireGuardPeer"
#define COMMAND_LIST_WIREGUARD_PEERS "listWireGuardPeers"
#define COMMAND_GET_WIREGUARD_IP "getWireGuardIp"

// WireGuard Arguments
#define COMMAND_ARG_HOST "host"
#define COMMAND_ARG_NAME "name"
#define COMMAND_ARG_VPN_IP "vpnIp"
#define COMMAND_ARG_MAC "mac"
#define COMMAND_ARG_DUAL_BOOT "dualBoot"
#define COMMAND_ARG_PRIVATE_KEY "privateKey"

// WireGuard Constants
#define WG_SERVER_IP "31.133.102.195"
#define WG_SERVER_USER "root"
#define WG_VPN_SUBNET "10.0.0."
#define WG_LEADER_IP "10.0.0.1"

#define COMMAND_ARG_DEVICE_PATH_REGEX "devicePathRegex"
#define COMMAND_ARG_IS_KEYBOARD "isKeyboard"
#define COMMAND_ARG_ACTION "action"
#define COMMAND_ARG_STRING "string"
#define COMMAND_ARG_PORT "port"
#define COMMAND_ARG_MESSAGE "message"

#define COMMAND_ARG_TYPE "type"
#define COMMAND_ARG_CODE "code"
#define COMMAND_ARG_WINDOW_TITLE "windowTitle"
#define COMMAND_ARG_WM_CLASS "wmClass"
#define COMMAND_ARG_WM_INSTANCE "wmInstance"
#define COMMAND_ARG_WINDOW_ID "windowId"
#define COMMAND_ARG_URL "url"
#define EVSIEVE_RANDOM_VAR "randomVar"
#define EVSIEVE_STANDARD_ERR_FILE "evsieveErr.log"
#define EVSIEVE_STANDARD_OUTPUT_FILE "evsieveOutput.log"
#define wmClassTerminal "gnome-terminal-server"
#define wmClassCode "code"
#define wmClassChrome "google-chrome"
#define wmClassAntigravity "Antigravity"
#define CODE_FOR_APP_CODES "codeForAppCodes"
#define VALUE_FOR_APP_CODES "100"
#define CODE_FOR_DEFAULT "codeForDefault"
#define VALUE_FOR_DEFAULT "101"
#define CODE_FOR_CODE "codeForCode"
#define VALUE_FOR_CODE "102"
#define CODE_FOR_GNOME_TERMINAL "codeForGnomeTerminal"
#define VALUE_FOR_GNOME_TERMINAL "103"
#define CODE_FOR_GOOGLE_CHROME "codeForGoogleChrome"
#define VALUE_FOR_GOOGLE_CHROME "104"
#define CODE_FOR_CNTRL_V "codeForCntrlV"
#define VALUE_FOR_CNTRL_V "205"
#define CODE_FOR_G1 "codeForG1"
#define VALUE_FOR_G1 "301"
#define CODE_FOR_G2 "codeForG2"
#define VALUE_FOR_G2 "302"
#define CODE_FOR_G3 "codeForG3"
#define VALUE_FOR_G3 "303"
#define CODE_FOR_G4 "codeForG4"
#define VALUE_FOR_G4 "304"
#define CODE_FOR_G5 "codeForG5"
#define VALUE_FOR_G5 "305"
#define CODE_FOR_G6 "codeForG6"
#define VALUE_FOR_G6 "306"
// Log Categories
#define LOG_NONE 0
#define LOG_INPUT (1 << 0)
#define LOG_WINDOW (1 << 1)
#define LOG_AUTOMATION (1 << 2)
#define LOG_CORE (1 << 3)
#define LOG_MACROS (1 << 4)
#define LOG_NETWORK (1 << 5)
#define LOG_CHROME (1 << 6)
#define LOG_TERMINAL (1 << 7)
#define LOG_INPUT_DEBUG (1 << 8)
#define LOG_ALL                                                                \
  (LOG_INPUT | LOG_WINDOW | LOG_AUTOMATION | LOG_CORE | LOG_MACROS |           \
   LOG_NETWORK | LOG_CHROME | LOG_TERMINAL | LOG_INPUT_DEBUG)
#define KEY_PRESS 1
#define KEY_RELEASE 0
#define KEY_REPEAT 2
#define WITHHOLD_YES true
#define WITHHOLD_NO false
#define KEY_REPEAT_BREAKS_YES true
#define KEY_REPEAT_BREAKS_NO false
#define NOISE_BREAKS_YES true
#define NOISE_BREAKS_NO false
// DBus Signal for proactive tracking
#define DBUS_SIGNAL_COMMAND                                                    \
  "/usr/bin/gdbus emit --system --object-path /com/automatelinux/daemon "      \
  "--signal "                                                                  \
  "com.automatelinux.daemon.Ready"
#endif // CONSTANTS_H
