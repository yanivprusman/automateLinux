#!/bin/bash
set -e

INSTALL_DIR="/opt/automateLinux"
echo "Starting AutomateLinux User Configuration..."

# 1. Determine User
TARGET_USER=$(whoami)
USER_HOME=$HOME

if [ "$TARGET_USER" == "root" ]; then
    echo "Warning: You are running this as root."
    echo "This script is intended to be run by the user who wants to use AutomateLinux."
    echo "Do you want to proceed? (y/N)"
    read -r CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo "  User: $TARGET_USER"
echo "  Home: $USER_HOME"
echo "  Source Installation: $INSTALL_DIR"

if [ ! -d "$INSTALL_DIR" ]; then
    echo "Error: AutomateLinux is not installed at $INSTALL_DIR."
    exit 1
fi

# 2. Configure ~/.bashrc
BASHRC_LOC="$USER_HOME/.bashrc"
BACKUP_BASHRC="$USER_HOME/.bashrc.bakup"

echo "Configuring $BASHRC_LOC..."

if [ -L "$BASHRC_LOC" ]; then
    CURRENT_TARGET=$(readlink -f "$BASHRC_LOC")
    if [ "$CURRENT_TARGET" == "$INSTALL_DIR/terminal/bashrc" ]; then
        echo "  ~/.bashrc is already correctly linked."
    else
        echo "  Updating ~/.bashrc symlink..."
        mv "$BASHRC_LOC" "$BACKUP_BASHRC"
        ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
        echo "  Backed up old bashrc to $BACKUP_BASHRC"
    fi
elif [ -f "$BASHRC_LOC" ]; then
    echo "  Backing up existing ~/.bashrc..."
    mv "$BASHRC_LOC" "$BACKUP_BASHRC"
    ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
else
    echo "  Creating ~/.bashrc symlink..."
    ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
fi

# 3. Configure Tmux
echo "Configuring Tmux..."
TMUX_CONF="$USER_HOME/.tmux.conf"
TMUX_TARGET="$INSTALL_DIR/terminal/tmux.conf"
if [ -f "$TMUX_TARGET" ]; then
    if [ -L "$TMUX_CONF" ] && [ "$(readlink -f "$TMUX_CONF")" == "$TMUX_TARGET" ]; then
        echo "  ~/.tmux.conf is already linked."
    else
        if [ -f "$TMUX_CONF" ] || [ -L "$TMUX_CONF" ]; then
            mv "$TMUX_CONF" "${TMUX_CONF}.bakup"
            echo "  Backed up old tmux.conf"
        fi
        ln -s "$TMUX_TARGET" "$TMUX_CONF"
        echo "  Linked ~/.tmux.conf"
    fi
fi

# 4. Configure GNOME Extensions
echo "Configuring GNOME Extensions..."
EXT_DIR="$USER_HOME/.local/share/gnome-shell/extensions"
mkdir -p "$EXT_DIR"

EXTENSIONS=("clock@ya-niv.com" "active-window-tracker@example.com" "window-selector@ya-niv.com")

for EXT in "${EXTENSIONS[@]}"; do
    SRC="$INSTALL_DIR/gnomeExtensions/$EXT"
    DEST="$EXT_DIR/$EXT"
    
    if [ -d "$SRC" ]; then
        if [ -L "$DEST" ] && [ "$(readlink -f "$DEST")" == "$SRC" ]; then
            echo "  Extension $EXT already linked."
        elif [ -e "$DEST" ]; then
            echo "  Warning: $DEST already exists and is not the correct link. Skipping to avoid data loss."
        else
            ln -s "$SRC" "$DEST"
            echo "  Linked extension $EXT"
        fi
    else
        echo "  Warning: Extension source $SRC not found."
    fi
done

# 5. Configure Autostart
echo "Configuring Autostart..."
AUTOSTART_DIR="$USER_HOME/.config/autostart"
AUTOSTART_TARGET="$INSTALL_DIR/autostart"

if [ -d "$AUTOSTART_TARGET" ]; then
    if [ -L "$AUTOSTART_DIR" ] && [ "$(readlink -f "$AUTOSTART_DIR")" == "$AUTOSTART_TARGET" ]; then
        echo "  ~/.config/autostart is already correctly linked."
    else
        if [ -e "$AUTOSTART_DIR" ]; then
            echo "  Backing up existing autostart directory..."
            mv "$AUTOSTART_DIR" "${AUTOSTART_DIR}.bakup"
        fi
        ln -s "$AUTOSTART_TARGET" "$AUTOSTART_DIR"
        echo "  Linked ~/.config/autostart to $AUTOSTART_TARGET"
    fi
fi

echo "--------------------------------------------------------"
echo "User Configuration Complete!"
echo "Please restart your terminal or run 'source ~/.bashrc' to apply changes."
echo "Verify connectivity with: d ping"
echo "--------------------------------------------------------"
