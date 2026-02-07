# Project Overview

This document provides an overview of the `automateLinux` project, a suite of custom automations designed to enhance and customize a Linux desktop environment. The project integrates various components, including a C++ daemon for system-level operations, a comprehensive set of shell scripts for terminal customization.

## Project Overview

The `automateLinux` project aims to provide a highly personalized and automated Linux experience. Its core components work in conjunction to manage system resources, streamline workflows, and enable advanced input device control.

*   **C++ Daemon (`daemon/`)**: A central background service responsible for system-level tasks. It discovers and manages input device paths (keyboard, mouse), maintains a key-value store (`KVTable`), handles directory history, and facilitates inter-process communication via UNIX domain sockets. It exposes a command interface for clients.
*   **Terminal Environment Customization (`terminal/`)**: A modular collection of Bash scripts that extensively customize the user's shell environment. This includes setting environment variables, defining aliases and functions, configuring key bindings, managing the prompt (`PS1`, `PROMPT_COMMAND`), and enabling dynamic behaviors within the terminal.
*   **Desktop Integration (`applications/`, `autostart/`, `desktop/`, `gnomeExtensions/`)**: Contains `.desktop` files for application launchers, autostart configurations, and custom Gnome Shell extensions to integrate automations directly into the graphical desktop environment.

## Installation

AutomateLinux is designed to be installed as a system-wide service rooted in `/opt/automateLinux/`.

### 1. System-Wide Installation (Root)

You can install the project cloning directly to `/opt`.
```bash
sudo git clone https://github.com/yanivprusman/automateLinux.git /opt/automateLinux
cd /opt/automateLinux
sudo ./install.sh
```

### 2. User Configuration (Per User)

Each user who wants to use the system must link their shell configuration to the system installation.

```bash
/opt/automateLinux/user_install.sh
```
*This backs up your existing `~/.bashrc` and links it to the managed configuration.*

### 3. Verification

After installation, reload your shell (`. ~/.bashrc`) and verify:

```bash
d ping          # Should return "pong"
```

## Development Conventions

*   **Modularity**: The project emphasizes modularity, particularly within the `terminal/` scripts, where concerns like aliases, functions, and environment variables are separated into distinct files.
*   **Environment Variables**: Extensive use of `AUTOMATE_LINUX_` prefixed environment variables for managing paths, configurations, and inter-script communication.
*   **Daemon-Client Architecture**: A clear client-server model for system services, leveraging UNIX sockets for efficient and secure inter-process communication.
*   **Systemd Integration**: The C++ daemon is managed as a `systemd` service, ensuring it starts automatically and is properly managed by the operating system.
*   **External Dependencies**: The C++ daemon relies on `SQLite3` for data storage and `systemd` for service management. 


installation instructions:
sudo git clone https://github.com/yanivprusman/automateLinux.git /opt/automateLinux
cd /opt/automateLinux
sudo ./install.sh

