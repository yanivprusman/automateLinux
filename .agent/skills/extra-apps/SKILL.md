---
name: extra-apps
description: Instructions for finding and managing extra/external applications like CAD and Public Transportation.
---

# Extra Applications

The AutomateLinux ecosystem integrates several external applications stored directly in the `extraApps/` directory.

## Registered Apps

| App ID | Display Name | Has Server | Dev Path | Prod Path |
|--------|--------------|------------|----------|-----------|
| `cad` | CAD Application | No | `/opt/automateLinux/extraApps/cad` | `/opt/prod/cad` |
| `pt` | Public Transportation | No | `/opt/automateLinux/extraApps/publicTransportation` | `/opt/prod/publicTransportation` |

## App Lifecycle Commands

### List and Status

```bash
d listApps                        # List all registered apps
d appStatus                       # Show status of all apps
d appStatus --app cad             # Show status of specific app
```

### Start/Stop/Restart

```bash
d startApp --app <name> --mode <prod|dev>     # Start app services
d stopApp --app <name> --mode <prod|dev|all>  # Stop app services
d restartApp --app <name> --mode <prod|dev>   # Restart app services
```

### Enable/Disable for Boot

```bash
d enableApp --app <name> --mode <prod|dev>    # Enable services at boot (systemctl enable)
d disableApp --app <name> --mode <prod|dev|all>  # Disable services from boot
```

### Build and Dependencies

```bash
# Build C++ server components (cmake && make)
d buildApp --app cad --mode prod

# Install npm dependencies
d installAppDeps --app cad --mode dev                        # Install all components
d installAppDeps --app cad --mode dev --component client     # Client only
```

## Port Management

App ports are managed centrally by the daemon.

### Current Standard Mappings

| App | Mode | Port | Description |
|-----|------|------|-------------|
| **CAD** | Prod | 3000 | CAD frontend production |
| **CAD** | Dev | 3001 | CAD frontend development |
| **PT** | Prod | 3002 | Public Transportation production |
| **PT** | Dev | 3003 | Public Transportation development |

### Updating Ports

```bash
d setPort --key pt-dev --value 3005   # Update port assignment
d getPort --key cad-dev               # Query current port
d listPorts                           # List all port mappings
```

## Production Versions

Production deployments are git worktrees located at `/opt/prod/<appName>/`:

```bash
ls /opt/prod/
# cad/  publicTransportation/
```

To update a production deployment:
```bash
cd /opt/prod/cad
git fetch origin
git checkout <commit-or-tag>
d buildApp --app cad --mode prod           # Rebuild server
d installAppDeps --app cad --mode prod     # Reinstall deps
d restartApp --app cad --mode prod         # Restart services
```

## Starting Applications

### CAD Dev Server
```bash
/opt/automateLinux/extraApps/cad/open_app.sh
```
This starts the dev server on the daemon-registered port and opens Chrome in app mode.

### Public Transportation
```bash
d publicTransportationOpenApp   # Opens in browser
```

## App Assignment (Multi-Peer)

When multiple machines are connected via WireGuard VPN, the daemon tracks which peer is actively working on each extraApp to prevent git conflicts.

### Claiming an App

Use `gita` instead of `git add` when working in an extraApp directory:

```bash
cd /opt/automateLinux/extraApps/cad
gita .                    # Claims app, then runs git add
```

The `gita` function automatically:
1. Detects which extraApp you're in
2. Calls `d claimApp --app <name>` to register ownership
3. Warns if another peer already owns the app
4. Triggers VPS nginx to forward the dev port to your machine

### Manual Commands

```bash
d claimApp --app cad       # Claim exclusive work on cad
d releaseApp --app cad     # Release when done
d getAppOwner --app cad    # Check who owns an app
```

## Source Files

- **daemon/src/cmdApp.cpp**: App lifecycle management (start/stop/restart/build/install)
- **daemon/include/cmdApp.h**: AppConfig struct and function declarations
- **services/system/**: Systemd service files for each app
