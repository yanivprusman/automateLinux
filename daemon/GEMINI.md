# Daemon

The core C++ service of the AutomateLinux project. It handles input event mapping, manages the MySQL database for state persistence, and provides a UDS (Unix Domain Socket) for communication with extensions and terminal scripts.

## Core Responsibilities

- **Input Management**: Grabs keyboard and mouse devices, intercepts events, and applies macros/mappings.
- **Database Management**: Manages MySQL connections and tables for terminal history, sessions, and configuration.
- **IPC**: Listens on a Unix Domain Socket (`/run/automatelinux/automatelinux-daemon.sock`) for commands from other components.
- **Terminal Integration**: Tracks directory history and provides navigation commands.

## Key Directories

- **[src](file:///home/yaniv/coding/automateLinux/daemon/src)**: Implementation files (.cpp).
- **[include](file:///home/yaniv/coding/automateLinux/daemon/include)**: Header files (.h).
- **[tests](file:///home/yaniv/coding/automateLinux/daemon/tests)**: Unit tests for core components.

## Utility Scripts

- `build.sh`: Compiles the daemon using CMake.
- `daemon.service`: Systemd service definition for the daemon.
- `showDb.sh`: Utility to query the MySQL database.
- `getDaemonPID.sh`: Helper to find the running daemon's process ID.
