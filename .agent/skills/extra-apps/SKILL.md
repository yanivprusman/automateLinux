---
name: extra-apps
description: Instructions for finding and managing extra/external applications like CAD and Public Transportation.
---

# Extra Applications

The AutomateLinux ecosystem integrates several external applications stored directly in the `extraApps/` directory.

## Locating Applications

### The `extraApps/` Directory
Applications are stored directly in `/opt/automateLinux/extraApps/`:

```bash
ls -l /opt/automateLinux/extraApps
```

This directory contains the actual source code (not symlinks):
- **`cad`**: CAD Shed Generator - `/opt/automateLinux/extraApps/cad/`
- **`loom`**: Screen recording integration - `/opt/automateLinux/extraApps/loom/`
- **`publicTransportation`**: Public transport app - `/opt/automateLinux/extraApps/publicTransportation/`

### Production Versions
Production deployments are git worktrees located at `/opt/prod/<appName>/` (e.g., `/opt/prod/cad/`).

## Port Management
External apps often run local development servers. Their ports are managed centrally by the daemon.

### Current Standard Mappings
| App | Mode | Standard Port |
| :--- | :--- | :--- |
| **CAD** | Production | 3000 |
| **CAD** | Development | 3001 |
| **PT** | All | 3002 |

### Updating Ports
If you move an app to a different port, update the daemon so macros and proxies continue to work:

```bash
d setPort --key pt --value 3005
```

## Starting Applications

### CAD Dev Server
```bash
/opt/automateLinux/extraApps/cad/open_app.sh
```
This starts the dev server on the daemon-registered port (key: `cad-dev`) and opens Chrome in app mode.

### Loom
```bash
d restartLoom   # Starts server + client + auto-selects screen
d stopLoom
d isLoomActive
```

### Public Transportation
```bash
d publicTransportationOpenApp   # Opens in browser
```

## Related Commands
- `d listPorts`: See all currently registered app ports.
- `d publicTransportationOpenApp`: Opens the PT app in Chrome based on the registered port.
- `d restartLoom`: Management for the Loom screen recording integration.

