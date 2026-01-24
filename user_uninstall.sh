#!/bin/bash
set -e

INSTALL_DIR="/opt/automateLinux"
TARGET_FILE="$HOME/.bashrc"
LINK_TARGET="$INSTALL_DIR/terminal/bashrc"

echo "Uninstalling AutomateLinux user configuration..."

# 1. Check if .bashrc is actually our symlink
if [ -L "$TARGET_FILE" ]; then
    CURRENT_LINK=$(readlink -f "$TARGET_FILE")
    if [ "$CURRENT_LINK" == "$LINK_TARGET" ]; then
        echo "Found AutomateLinux symlink at $TARGET_FILE"
        rm "$TARGET_FILE"
        echo "Symlink removed."
        
        # 2. Look for backups
        # Find the latest backup file matching usage in user_install.sh: .bashrc.bak.YYYY-MM-DD_HH-MM-SS
        # We sort by name (which works for ISO dates) and pick the last one.
        LATEST_BACKUP="$USER_HOME/.bashrc.bakup"
        
        if [ -n "$LATEST_BACKUP" ] && [ -f "$LATEST_BACKUP" ]; then
            echo "Restoring backup: $LATEST_BACKUP"
            mv "$LATEST_BACKUP" "$TARGET_FILE"
            echo "Restored original $TARGET_FILE."
        else
            echo "Warning: No backup file found matching $TARGET_FILE.bak.*"
            echo "You may need to manually recreate your .bashrc."
        fi
    else
        echo "Error: $TARGET_FILE is a symlink but points to $CURRENT_LINK, not $LINK_TARGET."
        echo "Aborting uninstall to avoid deleting files I didn't create."
        exit 1
    fi
elif [ -f "$TARGET_FILE" ]; then
    echo "Error: $TARGET_FILE is a regular file, not a symlink to AutomateLinux."
    echo "It seems AutomateLinux is not installed for this user (or was already removed)."
    exit 1
else
    echo "Error: $TARGET_FILE does not exist."
    # Not exiting here, to allow cleanup of other components if bashrc was manually tampered
fi

# 3. Cleanup Tmux
TMUX_CONF="$HOME/.tmux.conf"
TMUX_TARGET="$INSTALL_DIR/terminal/tmux.conf"
if [ -L "$TMUX_CONF" ] && [ "$(readlink -f "$TMUX_CONF")" == "$TMUX_TARGET" ]; then
    rm "$TMUX_CONF"
    echo "Removed ~/.tmux.conf link."
    if [ -f "${TMUX_CONF}.bakup" ]; then
        mv "${TMUX_CONF}.bakup" "$TMUX_CONF"
        echo "Restored original tmux.conf"
    fi
fi

# 4. Cleanup GNOME Extensions
EXT_DIR="$HOME/.local/share/gnome-shell/extensions"
EXTENSIONS=("clock@ya-niv.com" "active-window-tracker@example.com" "window-selector@ya-niv.com")
for EXT in "${EXTENSIONS[@]}"; do
    DEST="$EXT_DIR/$EXT"
    SRC="$INSTALL_DIR/gnomeExtensions/$EXT"
    if [ -L "$DEST" ] && [ "$(readlink -f "$DEST")" == "$SRC" ]; then
        rm "$DEST"
        echo "Removed extension link: $EXT"
    fi
done

# 5. Cleanup Autostart
AUTOSTART_DIR="$HOME/.config/autostart"
AUTOSTART_TARGET="$INSTALL_DIR/autostart"
if [ -L "$AUTOSTART_DIR" ] && [ "$(readlink -f "$AUTOSTART_DIR")" == "$AUTOSTART_TARGET" ]; then
    rm "$AUTOSTART_DIR"
    echo "Removed ~/.config/autostart link."
    if [ -e "${AUTOSTART_DIR}.bakup" ]; then
        mv "${AUTOSTART_DIR}.bakup" "$AUTOSTART_DIR"
        echo "Restored original ~/.config/autostart directory"
    fi
fi

echo "User uninstallation complete."
