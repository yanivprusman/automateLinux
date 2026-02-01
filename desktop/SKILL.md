# Desktop Launcher Management

This guide explains how to create, edit, and deploy desktop launchers (.desktop files) in the automateLinux project.

## Directory Structure

- **Source**: `/opt/automateLinux/desktop/*.desktop` - Canonical launcher files in the repository
- **Deployed**: `~/Desktop/*.desktop` - User's desktop (auto-synced on each new terminal session)

Files are automatically copied from source to deployed location via [terminal/myBashrc.sh](../terminal/myBashrc.sh#L23-L25) every time a new shell is started.

## Auto-Deployment Mechanism

Desktop launchers deploy automatically without manual intervention:

**How it works**:
1. Every new terminal session sources `~/.bashrc` → `terminal/bashrc` → `terminal/myBashrc.sh`
2. myBashrc.sh executes: `cp "${AUTOMATE_LINUX_DIR}desktop/"*.desktop ~/Desktop/`
3. All .desktop files sync from repo to desktop instantly

**Key implications**:
- **Always edit source files** in `/opt/automateLinux/desktop/` (repo-tracked)
- **Never edit deployed files** in `~/Desktop/` (overwritten on next terminal)
- **Trigger sync**: Open new terminal or run `cp /opt/automateLinux/desktop/*.desktop ~/Desktop/`

## Desktop Entry Format

`.desktop` files follow the [freedesktop.org Desktop Entry Specification](https://specifications.freedesktop.org/desktop-entry-spec/latest/):

```ini
[Desktop Entry]
Version=1.0
Name=Display Name
Comment=Tooltip description
Exec=command to execute
Icon=/path/to/icon.svg
Terminal=false
Type=Application
Categories=System;
```

### Key Fields

| Field | Description | Example |
|-------|-------------|---------|
| `Name` | Display name shown under icon | `Shutdown` |
| `Comment` | Tooltip text on hover | `Shutdown the system` |
| `Exec` | Command to execute when clicked | `bash -c 'systemctl poweroff'` |
| `Icon` | Path to icon image | `/usr/share/icons/...` or `/opt/automateLinux/images/...` |
| `Terminal` | Whether to open in terminal | `false` (GUI action) or `true` (shows terminal) |
| `Type` | Entry type | `Application` (standard for launchers) |
| `Categories` | Menu categories | `System;` or `Utility;Development;` |

## Executing Terminal Functions

To use functions from `terminal/functions/*.sh` in a desktop launcher, source the bashrc first:

```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && functionName"
```

### Example: Shutdown Launcher

```ini
[Desktop Entry]
Version=1.0
Name=Shutdown
Comment=Shutdown the system
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && shutDown"
Icon=/usr/share/icons/Humanity/actions/32/system-shutdown.svg
Terminal=false
Type=Application
Categories=System;
```

The `shutDown` function is defined in [terminal/functions/system.sh](../terminal/functions/system.sh):
```bash
shutDown() {
    systemctl poweroff
}
export -f shutDown
```

### Example: Daemon Command Launcher

```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && d publicTransportationOpenApp"
```

## Creating a New Launcher

1. **Define the function** (if needed) in appropriate file under `terminal/functions/`:
   ```bash
   # In terminal/functions/system.sh
   mySystemAction() {
       echo "Performing action..."
       # your logic here
   }
   export -f mySystemAction
   ```

2. **Create the .desktop file** in `/opt/automateLinux/desktop/`:
   ```bash
   cd /opt/automateLinux/desktop
   nano myaction.desktop
   ```

3. **Set executable permission**:
   ```bash
   chmod +x /opt/automateLinux/desktop/myaction.desktop
   ```

4. **Deploy automatically**: Open a new terminal (auto-deployment happens via bashrc)

5. **Mark as trusted** (GNOME): Right-click → "Allow Launching"

## Icon Guidelines

### Icon Theme Names (Recommended)
Use icon theme names for portability across different systems and icon themes. The desktop environment automatically resolves these to the correct icon:

```ini
Icon=system-shutdown    # Shutdown/power off
Icon=system-reboot      # Reboot/restart
Icon=system-lock-screen # Lock screen
Icon=system-log-out     # Log out
Icon=application-exit   # Exit/quit
```

**Benefits**:
- Works across different Linux distributions and icon themes
- Automatically adapts to user's theme (dark/light/custom)
- No need to check if icon file exists

### Custom Icons
Store custom icons in `/opt/automateLinux/images/` for project-specific launchers:
```ini
Icon=/opt/automateLinux/images/myicon.png
```

**Use absolute paths** starting with `/opt/automateLinux/images/` (not `~/` or relative paths).

Supported formats: PNG, SVG, JPG. Recommended size: 48x48 or 256x256 pixels.

## Common Patterns

### Direct System Command
```ini
Exec=systemctl poweroff
```

### Bash Command
```ini
Exec=bash -c 'systemctl reboot'
```

### Terminal Function (Single Command)
```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && d ping"
```

### Terminal Function (Multiple Commands)
```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && d ping && notify-send 'Daemon pinged'"
```

### With Terminal Output (set Terminal=true)
```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && d getPeerStatus"
Terminal=true
```

## Deployment Workflow

### Automatic Deployment (Default)

Desktop launchers are **automatically synced** from `/opt/automateLinux/desktop/` to `~/Desktop/` every time you open a new terminal.

**Implementation**: [terminal/myBashrc.sh:23-25](../terminal/myBashrc.sh#L23-L25)
```bash
if [ -d "${AUTOMATE_LINUX_DIR}desktop" ]; then
    cp "${AUTOMATE_LINUX_DIR}desktop/"*.desktop ~/Desktop/ 2>/dev/null || true
fi
```

**Workflow**:
1. Edit launcher in `/opt/automateLinux/desktop/`
2. Open a new terminal (triggers auto-deployment)
3. Desktop icons update automatically

### Manual Deployment (Optional)

If you need immediate deployment without opening a new terminal:

```bash
# Deploy all launchers
cp /opt/automateLinux/desktop/*.desktop ~/Desktop/

# Or deploy a specific launcher
cp /opt/automateLinux/desktop/shutDown.desktop ~/Desktop/

# Make executable if needed
chmod +x ~/Desktop/*.desktop
```

## Troubleshooting

### "File is not marked as executable"
```bash
chmod +x ~/Desktop/myaction.desktop
```

### "Untrusted application launcher" (GNOME)
Right-click the icon → "Allow Launching" or:
```bash
gio set ~/Desktop/myaction.desktop metadata::trusted true
```

### Command not found
Ensure you're sourcing bashrc when using terminal functions:
```ini
Exec=bash -c "source /opt/automateLinux/terminal/bashrc && myFunction"
```

### Icon not showing
- Verify icon path exists: `ls -l /path/to/icon.svg`
- Use absolute paths, not relative paths
- Try a system icon: `Icon=system-shutdown`

## Best Practices

1. **Source Control**: Always edit `/opt/automateLinux/desktop/*.desktop`, never edit `~/Desktop/*.desktop` directly (changes will be overwritten)
2. **Auto-Deployment**: After editing, open a new terminal to trigger auto-sync (or use manual cp if urgent)
3. **Functions First**: Define complex logic in `terminal/functions/*.sh`, not inline in Exec
4. **Comments**: Use meaningful Comment field for tooltip clarity
5. **Permissions**: Always set executable on source file: `chmod +x /opt/automateLinux/desktop/*.desktop`
6. **Icons**: Prefer system icons for consistency, custom icons for branding
7. **Testing**: Test launchers by double-clicking before committing
8. **Categories**: Use appropriate categories for menu organization

## Related Documentation

- [terminal/functions/system.sh](../terminal/functions/system.sh) - System power functions
- [CLAUDE.md](../CLAUDE.md) - Project overview and daemon commands
