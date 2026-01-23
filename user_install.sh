#!/bin/bash
set -ne

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
    echo "Please ask your administrator to run 'sudo $INSTALL_DIR/install.sh' first."
    exit 1
fi

# 2. Configure ~/.bashrc
BASHRC_LOC="$USER_HOME/.bashrc"
BACKUP_BASHRC="$USER_HOME/.bashrc.bak.$(date +%F_%H-%M-%S)"

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

echo "--------------------------------------------------------"
echo "User Configuration Complete!"
echo "Please restart your terminal or run 'source ~/.bashrc' to apply changes."
echo "Verify connectivity with: d ping"
echo "--------------------------------------------------------"
