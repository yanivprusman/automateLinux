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

# 1.5. Add user to coding group if needed
NEED_RELOGIN=false
if ! id -nG "$TARGET_USER" | grep -qw "coding"; then
    echo "Adding $TARGET_USER to 'coding' group..."
    sudo usermod -aG coding "$TARGET_USER"
    NEED_RELOGIN=true
fi

# 2. Configure ~/.bashrc
BASHRC_LOC="$USER_HOME/.bashrc"
BACKUP_BASHRC="$USER_HOME/.bashrc.bakup"

echo "Configuring $BASHRC_LOC..."

if [ -L "$BASHRC_LOC" ]; then
    CURRENT_TARGET=$(readlink -f "$BASHRC_LOC" 2>/dev/null || echo "")
    if [ "$CURRENT_TARGET" == "$INSTALL_DIR/terminal/bashrc" ]; then
        echo "  ~/.bashrc is already correctly linked."
    else
        echo "  Updating ~/.bashrc symlink (was: $(readlink "$BASHRC_LOC"))..."
        rm "$BASHRC_LOC"
        ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
        echo "  Done."
    fi
elif [ -f "$BASHRC_LOC" ]; then
    echo "  Backing up existing ~/.bashrc..."
    mv "$BASHRC_LOC" "$BACKUP_BASHRC"
    ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
else
    echo "  Creating ~/.bashrc symlink..."
    ln -s "$INSTALL_DIR/terminal/bashrc" "$BASHRC_LOC"
fi

# 3. Configure .profile
echo "Configuring ~/.profile..."
PROFILE_LOC="$USER_HOME/.profile"
PROFILE_TARGET="$INSTALL_DIR/profile/.profile"
if [ -f "$PROFILE_TARGET" ]; then
    if [ -L "$PROFILE_LOC" ]; then
        CURRENT_TARGET=$(readlink -f "$PROFILE_LOC" 2>/dev/null || echo "")
        if [ "$CURRENT_TARGET" == "$PROFILE_TARGET" ]; then
            echo "  ~/.profile is already correctly linked."
        else
            echo "  Updating ~/.profile symlink..."
            rm "$PROFILE_LOC"
            ln -s "$PROFILE_TARGET" "$PROFILE_LOC"
            echo "  Done."
        fi
    elif [ -f "$PROFILE_LOC" ]; then
        echo "  Backing up existing ~/.profile..."
        mv "$PROFILE_LOC" "${PROFILE_LOC}.bakup"
        ln -s "$PROFILE_TARGET" "$PROFILE_LOC"
    else
        echo "  Creating ~/.profile symlink..."
        ln -s "$PROFILE_TARGET" "$PROFILE_LOC"
    fi
fi

# 4. Configure Tmux
echo "Configuring Tmux..."
TMUX_CONF="$USER_HOME/.tmux.conf"
TMUX_TARGET="$INSTALL_DIR/terminal/tmux.conf"
if [ -f "$TMUX_TARGET" ]; then
    CURRENT_TARGET=$(readlink -f "$TMUX_CONF" 2>/dev/null || echo "")
    if [ "$CURRENT_TARGET" == "$TMUX_TARGET" ]; then
        echo "  ~/.tmux.conf is already linked."
    else
        if [ -f "$TMUX_CONF" ] || [ -L "$TMUX_CONF" ]; then
            rm "$TMUX_CONF"
        fi
        ln -s "$TMUX_TARGET" "$TMUX_CONF"
        echo "  Linked ~/.tmux.conf"
    fi
fi

# 5. Configure GNOME Extensions
echo "Configuring GNOME Extensions..."
EXT_DIR="$USER_HOME/.local/share/gnome-shell/extensions"
mkdir -p "$EXT_DIR"

EXTENSIONS=("clock@ya-niv.com" "active-window-tracker@example.com" "window-selector@ya-niv.com")

for EXT in "${EXTENSIONS[@]}"; do
    SRC="$INSTALL_DIR/gnomeExtensions/$EXT"
    DEST="$EXT_DIR/$EXT"

    if [ -d "$SRC" ]; then
        CURRENT_TARGET=$(readlink -f "$DEST" 2>/dev/null || echo "")
        if [ "$CURRENT_TARGET" == "$SRC" ]; then
            echo "  Extension $EXT already linked."
        elif [ -L "$DEST" ]; then
            echo "  Updating extension $EXT symlink..."
            rm "$DEST"
            ln -s "$SRC" "$DEST"
        elif [ -e "$DEST" ]; then
            echo "  Warning: $DEST already exists and is not a symlink. Skipping."
        else
            ln -s "$SRC" "$DEST"
            echo "  Linked extension $EXT"
        fi
    else
        echo "  Warning: Extension source $SRC not found."
    fi
done

# 6. Configure Autostart
echo "Configuring Autostart..."
AUTOSTART_DIR="$USER_HOME/.config/autostart"
AUTOSTART_TARGET="$INSTALL_DIR/autostart"

if [ -d "$AUTOSTART_TARGET" ]; then
    CURRENT_TARGET=$(readlink -f "$AUTOSTART_DIR" 2>/dev/null || echo "")
    if [ "$CURRENT_TARGET" == "$AUTOSTART_TARGET" ]; then
        echo "  ~/.config/autostart is already correctly linked."
    elif [ -L "$AUTOSTART_DIR" ]; then
        echo "  Updating autostart symlink..."
        rm "$AUTOSTART_DIR"
        ln -s "$AUTOSTART_TARGET" "$AUTOSTART_DIR"
        echo "  Done."
    elif [ -e "$AUTOSTART_DIR" ]; then
        echo "  Backing up existing autostart directory..."
        mv "$AUTOSTART_DIR" "${AUTOSTART_DIR}.bakup"
        ln -s "$AUTOSTART_TARGET" "$AUTOSTART_DIR"
        echo "  Done."
    else
        ln -s "$AUTOSTART_TARGET" "$AUTOSTART_DIR"
        echo "  Linked ~/.config/autostart"
    fi
fi

echo "--------------------------------------------------------"
echo "User Configuration Complete!"
echo "Verify connectivity with: d ping"
echo "--------------------------------------------------------"

if [ "$NEED_RELOGIN" = true ]; then
    echo ""
    echo "Activating coding group membership..."
    exec newgrp coding
fi
