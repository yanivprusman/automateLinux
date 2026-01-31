---
name: loom-management
description: Instructions for managing the Loom screen recording integration.
---

# Loom Management

This skill provides instructions for managing the Loom screen recording integration within the AutomateLinux suite.

## Architecture

Loom consists of:
- **Server**: C++ GStreamer-based screen capture (port 3500)
- **Client**: React/Vite web viewer (port 3005)

Loom runs in dev mode only. Both components run as systemd services managed by the daemon's app management system.

## Commands

### Generic App Commands (Preferred)

```bash
d startApp --app loom --mode dev      # Start both server + client
d stopApp --app loom --mode dev       # Stop dev services
d restartApp --app loom --mode dev    # Restart dev services
d appStatus --app loom                # Show service status
d enableApp --app loom --mode dev     # Enable for boot (systemctl enable)
d disableApp --app loom --mode dev    # Disable from boot
```

### Build and Dependencies

```bash
d buildApp --app loom --mode dev      # Build C++ server (cmake && make)
d installAppDeps --app loom --mode dev --component client  # npm install for client
d installAppDeps --app loom --mode dev --component server  # npm install for server
d installAppDeps --app loom --mode dev                     # Install all deps
```

### Legacy Shortcuts

These redirect to the generic app handlers:

```bash
d restartLoom                         # Alias for restartApp --app loom --mode dev
d stopLoom                            # Alias for stopApp --app loom --mode dev
d isLoomActive                        # Check port status (3500, 3005)
```

## Service Details

| Service | Description | Working Directory |
|---------|-------------|-------------------|
| `loom-server-dev` | Server on port 3500 | `/opt/automateLinux/extraApps/loom/server/build` |
| `loom-client-dev` | Client on port 3005 | `/opt/automateLinux/extraApps/loom/client` |

Services run as user `yaniv` (not root) to access the user's DBus session for GNOME/Mutter ScreenCast.

## Screen Selection Automation

The restart command can optionally trigger automatic screen selection:

**Automation Logic:**
- A Python script scans for a window titled "Share Screen" or with class "xdg-desktop-portal"
- **Safety**: It ONLY sends keystrokes if the window is explicitly found
- **Key Sequence**: Sends `TAB TAB ENTER TAB TAB ENTER` with 1ms delay between keys

## Troubleshooting

### DBus Connection Failed
If loom-server fails with "DBus connection failed: The connection is closed":
- Verify services run as user `yaniv`, not `root`
- Check Environment variables in service files include DBUS_SESSION_BUS_ADDRESS

### Binary Not Found (status=203/EXEC)
```bash
d buildApp --app loom --mode dev
```

### Missing node_modules
```bash
d installAppDeps --app loom --mode dev --component client
```

### Screen Share Window Not Detected
- Check if `xdg-desktop-portal-gnome` is running
- Use `d listWindows` to verify if the daemon can see the window

## Source Files

- **daemon/src/cmdApp.cpp**: Generic app management implementation
- **daemon/src/cmdLoom.cpp**: Loom-specific handlers (redirect to generic)
- **services/system/loom-*.service**: Systemd service definitions
- **extraApps/loom/server/**: C++ GStreamer server source
- **extraApps/loom/client/**: React/Vite client source
