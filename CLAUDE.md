# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

automateLinux is a suite of tools for personalizing and automating a Linux desktop environment:
- **C++ Daemon** (`daemon/`): Central background service managing input events, port registry, database state, and command dispatching via UNIX sockets
- **Terminal Environment** (`terminal/`): Modular Bash scripts for shell customization including shared directory history (`Ctrl+Up/Down`), custom prompt, aliases, and functions
- **Dashboard** (`dashboard/`): React/Vite web app for live logs, macro builder, and system configuration. Uses bridge.cjs to communicate with daemon
- **Chrome Extension** (`chromeExtensions/daemon/`): Browser state sync via Native Messaging (tracks active tabs, Ctrl+V focus+paste on ChatGPT)
- **GNOME Extensions** (`gnomeExtensions/`): Desktop integration - status menu for daemon control, active window tracking
- **VS Code Extensions** (`visualStudioCodeExtensions/`): Editor integrations for daemon monitoring, git workflows, and log viewing
- **Utilities** (`utilities/`): Standalone tools - termcontrol, sendKeysUInput, lastChanged, cleanBetween, emergencyRestore.sh
- **Extra Apps** (`extraApps/`): Standalone applications housed directly in this repo (cad, loom, publicTransportation) to simplify agent access.

## Build Commands

### Daemon (C++)
```bash
bd                    # Build daemon from anywhere (preferred)
```
The `bd` function changes to daemon dir, runs `source ./build.sh`, and returns. The build stops the service, compiles with CMake, deploys the binary, and restarts `daemon.service`.

### Dashboard
```bash
node dashboard/bridge.cjs &                      # Start bridge (port 3501)
cd dashboard && npm run dev -- --port 3007       # Start frontend (port 3007)
# Then open http://localhost:3007
```

### Extra Apps (CAD, Loom, PT)
```bash
# CAD Frontend
cd extraApps/cad/web && npm run dev -- -p $(d getPort --key cad-dev)

# Loom
d restartLoom                    # Starts server + client + auto-selects screen
d stopLoom
d isLoomActive
```

### C++ Utilities
```bash
cd utilities/<name> && ./build.sh
./build.sh -rebuild              # Full rebuild
```

### Create New C/C++ Project
```bash
prepareBuild -c myproject        # C project (C11)
prepareBuild -cpp myproject      # C++ project (C++17)
```

## Port Management

The daemon is the central authority for port assignments. Apps must query the daemon for their assigned port - they cannot choose ports freely. nginx and other services expect these daemon-managed ports.

```bash
d setPort --key <app> --value <port>    # Assign port
d getPort --key <app>                   # Query assigned port
d listPorts                             # List all port mappings
d deletePort --key <app>                # Remove port entry
```

### Port Allocation Scheme
- **3000-3499**: App prod/dev ports (2 per app, sequential)
- **3500+**: Special operational ports (WebSocket servers, bridges, etc.)

### Current Port Registry
| Key | Port | Description |
| :--- | :--- | :--- |
| cad-prod | 3000 | CAD app production |
| cad-dev | 3001 | CAD app development |
| pt-prod | 3002 | Public Transportation production |
| pt-dev | 3003 | Public Transportation development |
| loom-prod | 3004 | Loom client production |
| loom-dev | 3005 | Loom client development |
| dashboard-prod | 3006 | Dashboard frontend production |
| dashboard-dev | 3007 | Dashboard frontend development |
| loom-server | 3500 | Loom WebSocket stream server |
| dashboard-bridge | 3501 | Dashboard daemon bridge |

## Daemon Communication

```bash
d <command>                  # Send command (shorthand, auto-prepends 'send')
d help                       # List available commands

# Common commands
d ping
d listPorts
d enableKeyboard / disableKeyboard
d simulateInput --string "text"
d simulateInput --type 1 --code 30 --value 1   # Raw key event (EV_KEY, KEY_A, press)
d restartLoom / stopLoom / isLoomActive
d publicTransportationOpenApp
d registerLogListener                           # For live log streaming
```

## Adding a New Daemon Command

1. **Define constants** in `daemon/include/Constants.h`:
   ```cpp
   #define COMMAND_MY_ACTION "myAction"
   #define COMMAND_ARG_MY_PARAM "myParam"
   ```

2. **Implement handler** in `daemon/src/mainCommand.cpp`:
   ```cpp
   CmdResult handleMyAction(const json &command) {
       string param = command[COMMAND_ARG_MY_PARAM].get<string>();
       return CmdResult(0, "Success: " + param + "\n");
   }
   ```

3. **Register signature** in `COMMAND_REGISTRY` array

4. **Register dispatcher** in `COMMAND_HANDLERS` array

5. **Build and test**: `bd && d myAction --myParam "test"`

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         nginx                                    │
│              (routes to daemon-managed ports)                    │
└────────────────────────┬────────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────────┐
│                    C++ Daemon (systemd: daemon.service)          │
│  - UNIX socket: /run/automatelinux/automatelinux-daemon.sock     │
│  - Port registry for all apps                                    │
│  - MySQL for state persistence                                   │
│  - libevdev for input device grab/remap                          │
│  - uinput for key simulation                                     │
└────────────────────────┬────────────────────────────────────────┘
                         │
   ┌─────────┬───────────┼───────────┬───────────┬───────────┐
   ▼         ▼           ▼           ▼           ▼           ▼
┌───────┐ ┌─────────┐ ┌─────────┐ ┌───────┐ ┌───────┐ ┌──────────┐
│Term   │ │Dashboard│ │Chrome   │ │GNOME  │ │VS Code│ │Extra Apps│
│(bash) │ │(React)  │ │Extension│ │Ext    │ │Ext    │ │cad/loom  │
└───────┘ └─────────┘ └─────────┘ └───────┘ └───────┘ └──────────┘
```

### Key Source Files
- **daemon/src/mainCommand.cpp**: Command dispatcher (all daemon commands)
- **daemon/src/InputMapper.cpp**: Input event interception and remapping
- **daemon/src/DaemonServer.cpp**: UNIX socket server
- **daemon/src/DatabaseTableManagers.cpp**: SQLite table management
- **dashboard/bridge.cjs**: WebSocket/REST bridge to daemon for dashboard
- **terminal/functions/*.sh**: Modular bash functions

### Terminal Environment
Loads via `~/.bashrc` sourcing `terminal/bashrc`:
1. `firstThing.sh` - Sets `AUTOMATE_LINUX_*` environment variables
2. `myBashrc.sh` - Sources modules (colors, aliases, functions, bindings)
3. `functions/*.sh` - Modular functions (daemon.sh, git.sh, build.sh, etc.)

## Extra Apps

Applications in `extraApps/` are stored directly in this repo (not symlinks) to simplify AI agent access without extra permissions. Each app may have its own build system and CLAUDE.md/GEMINI.md for app-specific guidance.

**Production versions** are deployed to `../prod/<appName>` (i.e., `/home/yaniv/coding/prod/<appName>`).

## Environment Variables

All prefixed with `AUTOMATE_LINUX_`:
- `AUTOMATE_LINUX_DIR` - Root project directory
- `AUTOMATE_LINUX_SOCKET_PATH` - Daemon socket (`/run/automatelinux/automatelinux-daemon.sock`)
- `AUTOMATE_LINUX_DATA_DIR` - Persistent data directory
- `AUTOMATE_LINUX_DAEMON_DIR` - Daemon source directory
- `AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR` - Terminal function scripts

## Dependencies

### Daemon (C++)
CMake 3.10+, C++17, MySQL Connector/C++, JSONCpp, libevdev, systemd, Boost, cURL

### Dashboard
Node.js, React 19, Vite, TypeScript, WebSocket

## Emergency Recovery

If daemon crashes while holding input devices:
```bash
./utilities/emergencyRestore.sh
```
