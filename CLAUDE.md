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
- **Extra Apps** (`extraApps/`): Standalone applications housed directly in this repo (cad, publicTransportation) to simplify agent access.

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

### Extra Apps (CAD, PT)

Use daemon commands for all app lifecycle management:

```bash
# Generic app management (preferred)
d listApps                                          # List available apps
d appStatus                                         # Status of all apps
d appStatus --app cad                               # Status of specific app
d startApp --app cad --mode dev                     # Start app in dev mode
d stopApp --app cad --mode dev                      # Stop specific mode
d restartApp --app cad --mode dev                   # Restart app

# Build and dependencies
d buildApp --app cad --mode dev                     # Build C++ server component
d installAppDeps --app cad --component client       # Install client deps only
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
| dashboard-prod | 3006 | Dashboard frontend production |
| dashboard-dev | 3007 | Dashboard frontend development |
| dashboard-bridge | 3501 | Dashboard daemon bridge |
| peer-socket | 3502 | Daemon peer-to-peer TCP socket (WireGuard) |
| rdp | 3389 | RDP (gnome-remote-desktop) |

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
d publicTransportationOpenApp
d registerLogListener                           # For live log streaming

# App management commands
d listApps                                       # List registered apps
d appStatus [--app <name>]                       # Show app service status
d startApp --app <name> --mode <prod|dev>        # Start app services
d stopApp --app <name> --mode <prod|dev|all>     # Stop app services
d restartApp --app <name> --mode <prod|dev>      # Restart app services
d enableApp --app <name> --mode <prod|dev>       # Enable app for boot (systemctl enable)
d disableApp --app <name> --mode <prod|dev|all>  # Disable app from boot
d buildApp --app <name> --mode <prod|dev>        # Build C++ server component
d installAppDeps --app <name> [--component <x>]  # Install npm dependencies
d deployToProd --app <name> [--commit <hash>]    # Deploy dev to prod (ALWAYS use this)
d prodStatus --app <name>                        # Check if prod is clean/dirty
d cleanProd --app <name>                         # Discard uncommitted changes in prod

# Peer networking commands (all work from any peer)
d registerWorker                                 # Register as worker, connect to VPS
d getPeerStatus                                  # Show local role, connections
d listPeers                                      # List all registered peers
d getPeerInfo --peer <id>                        # Get info for specific peer
d deletePeer --peer <id>                         # Remove peer from registry
d execOnPeer --peer <id> --directory <path> --shellCmd <cmd>  # Remote exec
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
│(bash) │ │(React)  │ │Extension│ │Ext    │ │Ext    │ │cad/pt    │
└───────┘ └─────────┘ └─────────┘ └───────┘ └───────┘ └──────────┘
```

## Multi-Peer Networking

The daemon supports distributed operation across multiple machines connected via WireGuard VPN. This enables remote command execution across peers.

### VPN Environment

```
                    WireGuard VPN (10.0.0.0/24)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│   Worker      │     │    LEADER     │     │   Worker      │
│  10.0.0.4     │────▶│   10.0.0.1    │◀────│  10.0.0.2     │
│  (Laptop)     │     │   (VPS)       │     │  (Desktop)    │
└───────────────┘     └───────────────┘     └───────────────┘
        TCP:3502              TCP:3502              TCP:3502
```

- **VPS (Leader)**: 10.0.0.1 (public: 31.133.102.195) - nginx proxy host, peer registry
- **Desktop**: 10.0.0.2
- **Laptop**: 10.0.0.4
- **Port**: 3502 (peer TCP socket, bound to wg0 interface)

### Setup

**On the leader (VPS):**
```bash
d setPeerConfig --role leader --id vps
```

**On workers (Desktop, Laptop):**
```bash
d registerWorker    # Uses hostname as peer_id, connects to VPS (10.0.0.1)
```

Or with explicit ID: `d setPeerConfig --role worker --id laptop --leader 10.0.0.1`

Workers automatically connect to the leader on startup and register themselves.

### Remote Command Execution

> **⚠️ DO NOT USE SSH DIRECTLY.** Always use daemon commands for remote execution. SSH lacks directory context and causes repeated failures.

```bash
# ✅ CORRECT - Use daemon commands
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"
d remotePull --peer vps
d remoteBd --peer laptop
d remoteDeployDaemon --peer laptop

# ❌ WRONG - Never use raw SSH
ssh 10.0.0.1 "git pull"  # Fails: no directory context
```

**Note:** The argument is `--shellCmd` (not `--command`) to avoid collision with the JSON `"command"` key.

**If a peer is unreachable:** Ask the user to manually bootstrap it. Never use SSH directly.

### Key Commands

All peer commands work from **any peer** (leader or worker). Workers automatically forward requests to the leader when needed.

| Command | Description |
|---------|-------------|
| `d registerWorker` | Register as worker (uses hostname, connects to VPS) |
| `d setPeerConfig --role <role> --id <id> [--leader <ip>]` | Configure peer role (advanced) |
| `d getPeerStatus` | Show current peer configuration (local) |
| `d listPeers` | List all registered peers with status |
| `d getPeerInfo --peer <id>` | Get detailed info for a specific peer |
| `d deletePeer --peer <id>` | Remove a peer from the registry |
| `d dbSanityCheck` | Check/fix worker database (delete leader-only data) |
| `d execOnPeer --peer <id> --directory <path> --shellCmd <cmd>` | Execute shell command on remote peer |
| `d remotePull --peer <id>` | Git pull automateLinux on peer |
| `d remoteBd --peer <id>` | Build daemon on peer |
| `d remoteDeployDaemon --peer <id>` | Pull + build daemon on peer |

### Shell Helper Functions

| Function | Usage | Description |
|----------|-------|-------------|
| `getPeerIdByIp` | `getPeerIdByIp 10.0.0.1` | Lookup peer_id from IP address |
| `execOnPeerByIp` | `execOnPeerByIp 10.0.0.1 /path "cmd"` | Execute command on peer by IP |

### Key Source Files
- **daemon/src/mainCommand.cpp**: Command dispatcher (all daemon commands)
- **daemon/src/cmdPeer.cpp**: Peer networking command handlers (listPeers, deletePeer, execOnPeer, etc.)
- **daemon/src/cmdApp.cpp**: App lifecycle management (start/stop/restart/build/install)
- **daemon/src/InputMapper.cpp**: Input event interception and remapping
- **daemon/src/DaemonServer.cpp**: UNIX socket server + TCP peer socket handling
- **daemon/src/PeerManager.cpp**: Leader/worker connection management
- **daemon/src/DatabaseTableManagers.cpp**: MySQL table management (including peer_registry)
- **dashboard/bridge.cjs**: WebSocket/REST bridge to daemon for dashboard
- **terminal/functions/git.sh**: Git helpers including `gita` with behind-branch warning
- **terminal/functions/*.sh**: Modular bash functions

### Terminal Environment
Loads via `~/.bashrc` sourcing `terminal/bashrc`:
1. `firstThing.sh` - Sets `AUTOMATE_LINUX_*` environment variables
2. `myBashrc.sh` - Sources modules (colors, aliases, functions, bindings)
3. `functions/*.sh` - Modular functions (daemon.sh, git.sh, build.sh, etc.)

### System Utilities

| Function | Description |
|----------|-------------|
| `fixAutoLinuxPerms [path]` | Fix ownership (root:coding) and permissions (group-writable, setgid) on automateLinux directories |

## Permissions Setup

The repository uses `root:coding` ownership with group-writable permissions:
- All users who need write access should be in the `coding` group
- System-wide `umask 002` via PAM ensures new files are group-writable
- The `%coding` group has `NOPASSWD` sudo access for build/deploy commands

To fix permissions on a new machine:
```bash
fixAutoLinuxPerms                    # Fix /opt/automateLinux
fixAutoLinuxPerms /opt/prod/cad      # Fix specific path
```

## Extra Apps

Applications in `extraApps/` are stored directly in this repo (not symlinks) to simplify AI agent access without extra permissions. Each app may have its own build system and CLAUDE.md/GEMINI.md for app-specific guidance.

**Production versions** are git worktrees at `/opt/prod/<appName>`, checked out at specific commits (detached HEAD). To update prod, use `git worktree` commands to move to a new commit.

> **⚠️ NEVER DEVELOP IN PROD.** All development must happen in `extraApps/<appName>` (dev). The prod worktree at `/opt/prod/<appName>` must ONLY contain committed code - no local modifications ever.
>
> **Deployment workflow (use daemon commands only):**
> 1. Make changes in `extraApps/<appName>` and test with dev mode
> 2. Commit changes to the app's git repo
> 3. Deploy to prod: `d deployToProd --app <name>` (uses latest dev commit)
>    - Or specify commit: `d deployToProd --app <name> --commit <hash>`
>
> **Never use direct git commands on prod** - always use `d deployToProd`.

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
