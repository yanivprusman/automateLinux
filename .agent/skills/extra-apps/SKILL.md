---
name: extra-apps
description: Instructions for finding and managing extra/external applications like CAD and Public Transportation.
---

# Extra Applications

The AutomateLinux ecosystem integrates several external applications. These are typically stored outside the main `automateLinux` repository but are linked for easy access.

## Locating Applications

### The `extraApps/` Directory
The primary entry point for finding external apps is the `extraApps/` directory in the root of the `automateLinux` workspace.

```bash
ls -l ~/coding/automateLinux/extraApps
```

This directory contains symlinks to the actual source locations:
- **`cad`**: Points to `~/coding/cad/`
- **`publicTransportation`**: Points to `~/coding/publicTransportation/`

### Direct Paths
- **CAD App**: `/home/yaniv/coding/cad`
- **Public Transportation (PT) App**: `/home/yaniv/coding/publicTransportation`

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

## Editing and Permissions

> [!IMPORTANT]
> The user has granted explicit permission to edit files within the `extraApps/` symlinked directories.

You may modify, create, or delete files in `~/coding/cad`, `~/coding/loom`, and `~/coding/publicTransportation` without further permission, as they are considered integral parts of the AutomateLinux workspace.

## Related Commands
- `d listPorts`: See all currently registered app ports.
- `d publicTransportationOpenApp`: Opens the PT app in Chrome based on the registered port.
- `d restartLoom`: Management for the Loom screen recording integration.

