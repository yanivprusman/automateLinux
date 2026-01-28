# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

automateLinux is a suite of tools for personalizing and automating a Linux desktop environment:
- **C++ Daemon** (`daemon/`): Central background service managing input events, port registry, database state, peer networking (over WireGuard VPN), and command dispatching via UNIX sockets and TCP
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

# Peer networking commands
d setPeerConfig --role leader --id desktop     # Configure as leader
d setPeerConfig --role worker --id vps --leader 10.0.0.2  # Configure as worker
d getPeerStatus                                 # Show role, connections
d listPeers                                     # List registered peers

# App assignment (prevents git conflicts on extraApps)
d claimApp --app cad                           # Claim exclusive work on app
d releaseApp --app cad                         # Release app assignment
d listApps                                     # Show all app assignments
d getAppOwner --app cad                        # Check who owns an app
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

## Multi-Peer Networking

The daemon supports distributed operation across multiple machines connected via WireGuard VPN. This enables:
- **Centralized app assignment** - Prevent git conflicts by locking extraApps to specific peers
- **Dynamic port forwarding** - VPS nginx automatically routes dev ports to the active peer

### VPN Environment

```
                    WireGuard VPN (10.0.0.0/24)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│   Worker      │     │    LEADER     │     │   Worker      │
│  10.0.0.4     │────▶│   10.0.0.2    │◀────│  10.0.0.1     │
│  (Laptop)     │     │   (Desktop)   │     │  (VPS)        │
└───────────────┘     └───────────────┘     └───────────────┘
        TCP:3600              TCP:3600              TCP:3600
```

- **VPS**: 10.0.0.1 (public: 31.133.102.195) - nginx proxy host
- **Desktop (Leader)**: 10.0.0.2 - source of truth for app assignments
- **Laptop**: 10.0.0.4
- **Port**: 3600 (peer TCP socket, bound to wg0 interface)

### Setup

**On the leader (Desktop):**
```bash
d setPeerConfig --role leader --id desktop
```

**On workers (VPS, Laptop):**
```bash
d setPeerConfig --role worker --id vps --leader 10.0.0.2
d setPeerConfig --role worker --id laptop --leader 10.0.0.2
```

Workers automatically connect to the leader on startup and register themselves.

### App Assignment (Preventing Git Conflicts)

When working on extraApps (cad, loom, pt), the `gita` function automatically claims the app:

```bash
cd /opt/automateLinux/extraApps/cad
gita .                    # Calls 'd claimApp --app cad' before git add
# Output: "Claimed cad (dev port 3001), VPS forwarding to 10.0.0.2"
```

If another peer already owns the app:
```bash
gita .
# Output: "WARNING: cad is assigned to laptop since 2026-01-28 10:14:43"
# Prompt: "Continue anyway? [y/N]"
```

### Nginx Port Forwarding

When a peer claims an app, the leader notifies VPS to update nginx:

1. Laptop claims `cad` (dev port 3001)
2. Leader sends `updateNginxForward` to VPS
3. VPS creates `/etc/nginx/conf.d/daemon-forward-3001.conf`
4. External traffic to `http://VPS_PUBLIC_IP:3001` routes to laptop over VPN

### Key Commands

| Command | Description |
|---------|-------------|
| `d setPeerConfig --role <role> --id <id> [--leader <ip>]` | Configure peer role |
| `d getPeerStatus` | Show current peer configuration |
| `d listPeers` | List all registered peers with status |
| `d claimApp --app <name>` | Claim exclusive work on an extraApp |
| `d releaseApp --app <name>` | Release app assignment |
| `d listApps` | Show all app assignments |
| `d getAppOwner --app <name>` | Check who owns an app |

### Key Source Files
- **daemon/src/mainCommand.cpp**: Command dispatcher (all daemon commands)
- **daemon/src/InputMapper.cpp**: Input event interception and remapping
- **daemon/src/DaemonServer.cpp**: UNIX socket server + TCP peer socket handling
- **daemon/src/PeerManager.cpp**: Leader/worker connection management
- **daemon/src/DatabaseTableManagers.cpp**: MySQL table management (including peer_registry, app_assignments)
- **dashboard/bridge.cjs**: WebSocket/REST bridge to daemon for dashboard
- **terminal/functions/git.sh**: Git helpers including `gita` with app claiming
- **terminal/functions/*.sh**: Modular bash functions

### Terminal Environment
Loads via `~/.bashrc` sourcing `terminal/bashrc`:
1. `firstThing.sh` - Sets `AUTOMATE_LINUX_*` environment variables
2. `myBashrc.sh` - Sources modules (colors, aliases, functions, bindings)
3. `functions/*.sh` - Modular functions (daemon.sh, git.sh, build.sh, etc.)

## Extra Apps

Applications in `extraApps/` are stored directly in this repo (not symlinks) to simplify AI agent access without extra permissions. Each app may have its own build system and CLAUDE.md/GEMINI.md for app-specific guidance.

**Production versions** are git worktrees at `/opt/prod/<appName>`, checked out at specific commits (detached HEAD). To update prod, use `git worktree` commands to move to a new commit.

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
