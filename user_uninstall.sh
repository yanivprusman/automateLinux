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
        LATEST_BACKUP=$(ls -1 "$TARGET_FILE.bak."* 2>/dev/null | sort | tail -n 1)
        
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
    exit 1
fi

echo "User uninstallation complete."
