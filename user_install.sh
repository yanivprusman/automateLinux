#!/bin/bash
set -e

INSTALL_DIR="/opt/automateLinux"
MINIMAL_INSTALL=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --minimal|-m)
            MINIMAL_INSTALL=true
            shift
            ;;
        --help|-h)
            echo "Usage: ./user_install.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --minimal, -m    Minimal install for headless/VPS systems:"
            echo "                   - Skip GNOME extensions"
            echo "                   - Skip autostart configuration"
            echo "  --help, -h       Show this help message"
            exit 0
            ;;
    esac
done

if [ "$MINIMAL_INSTALL" = true ]; then
    echo "Starting AutomateLinux User Configuration (MINIMAL)..."
else
    echo "Starting AutomateLinux User Configuration..."
fi

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

# Ensure DBUS session bus is available (needed for grdctl, systemctl --user)
# When run remotely via daemon execOnPeer, these env vars are missing
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    UID_NUM=$(id -u)
    if [ -S "/run/user/$UID_NUM/bus" ]; then
        export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID_NUM/bus"
        export XDG_RUNTIME_DIR="/run/user/$UID_NUM"
    fi
fi

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

# 1.6. Ensure trap log files are world-writable
sudo chmod 666 "$INSTALL_DIR/data/trapErrLog.txt" "$INSTALL_DIR/data/trapErrLogBackground.txt" 2>/dev/null || true

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
if [ "$MINIMAL_INSTALL" = false ]; then
    echo "Configuring GNOME Extensions..."
    EXT_DIR="$USER_HOME/.local/share/gnome-shell/extensions"
    mkdir -p "$EXT_DIR"

    EXTENSIONS=("clock@ya-niv.com" "active-window-tracker@example.com" "window-selector@ya-niv.com")

    # Link shared lib directory (extensions use ../lib/ imports)
    LIB_SRC="$INSTALL_DIR/gnomeExtensions/lib"
    LIB_DEST="$EXT_DIR/lib"
    if [ -d "$LIB_SRC" ]; then
        CURRENT_TARGET=$(readlink -f "$LIB_DEST" 2>/dev/null || echo "")
        if [ "$CURRENT_TARGET" == "$LIB_SRC" ]; then
            echo "  Shared lib directory already linked."
        elif [ -L "$LIB_DEST" ]; then
            echo "  Updating lib symlink..."
            rm "$LIB_DEST"
            ln -s "$LIB_SRC" "$LIB_DEST"
        elif [ -e "$LIB_DEST" ]; then
            echo "  Warning: $LIB_DEST already exists and is not a symlink. Skipping."
        else
            ln -s "$LIB_SRC" "$LIB_DEST"
            echo "  Linked shared lib directory"
        fi
    fi

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
else
    echo "Skipping GNOME extensions (minimal install)..."
fi

# 6. Configure Autostart
if [ "$MINIMAL_INSTALL" = false ]; then
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
else
    echo "Skipping autostart configuration (minimal install)..."
fi

# 7. Configure GNOME Desktop Settings
if [ "$MINIMAL_INSTALL" = false ]; then
    echo "Configuring GNOME desktop settings..."
    gsettings set org.gnome.desktop.interface enable-hot-corners true
    echo "  ✓ Hot corners enabled"
else
    echo "Skipping GNOME desktop settings (minimal install)..."
fi

# 8. Configure GNOME Remote Desktop (RDP)
if [ "$MINIMAL_INSTALL" = false ]; then
    echo "Configuring GNOME Remote Desktop..."
    GRD_DIR="$USER_HOME/.local/share/gnome-remote-desktop"
    GRD_KEY="$GRD_DIR/rdp-tls.key"
    GRD_CERT="$GRD_DIR/rdp-tls.crt"

    # Check if gnome-remote-desktop is available
    if command -v grdctl >/dev/null 2>&1; then
        mkdir -p "$GRD_DIR"

        # Generate TLS certificates if they don't exist
        if [ ! -f "$GRD_KEY" ] || [ ! -f "$GRD_CERT" ]; then
            echo "  Generating TLS certificates..."
            openssl req -x509 -newkey rsa:2048 \
                -keyout "$GRD_KEY" \
                -out "$GRD_CERT" \
                -days 365 -nodes -subj "/CN=localhost" 2>/dev/null
            chmod 600 "$GRD_KEY"
            echo "  TLS certificates generated."
        else
            echo "  TLS certificates already exist."
        fi

        # Configure RDP (use default password, user should change it)
        echo "  Enabling RDP..."
        grdctl rdp enable 2>/dev/null || true
        grdctl rdp set-tls-key "$GRD_KEY" 2>/dev/null || true
        grdctl rdp set-tls-cert "$GRD_CERT" 2>/dev/null || true
        grdctl rdp disable-view-only 2>/dev/null || true

        # Check if credentials are set (shows "(null)" or "(hidden)" when set)
        # Empty CRED_STATUS means grdctl couldn't reach keyring — treat as needing credentials
        CRED_STATUS=$(grdctl status --show-credentials 2>/dev/null | grep -E "Username:|Password:" || true)
        if [ -z "$CRED_STATUS" ] || echo "$CRED_STATUS" | grep -q "(null)"; then
            echo "  Setting default RDP credentials..."
            CRED_OUTPUT=$(grdctl rdp set-credentials "$TARGET_USER" "changeme123" 2>&1)
            CRED_RESULT=$?

            if [ $CRED_RESULT -ne 0 ] || echo "$CRED_OUTPUT" | grep -qi "locked"; then
                echo "  Keyring locked or missing, resetting..."
                rm -f "$USER_HOME/.local/share/keyrings/login.keyring"
                CRED_OUTPUT=$(grdctl rdp set-credentials "$TARGET_USER" "changeme123" 2>&1)
                CRED_RESULT=$?
                if [ $CRED_RESULT -ne 0 ]; then
                    echo "  ⚠ WARNING: Could not set RDP credentials after keyring reset."
                    echo "  Try logging out and back in, then re-run: ./user_install.sh"
                else
                    echo "  Keyring reset and RDP credentials set (password: changeme123)"
                fi
            else
                echo "  RDP credentials set (password: changeme123)"
                echo "  Change with: grdctl rdp set-credentials USER NEWPASS"
            fi
        elif echo "$CRED_STATUS" | grep -q "(hidden)"; then
            echo "  RDP credentials already set."
        fi

        # Check if xrdp is blocking port 3389
        if systemctl is-active --quiet xrdp 2>/dev/null; then
            echo ""
            echo "  ⚠ WARNING: xrdp is running and blocking port 3389!"
            echo "  Run 'sudo install.sh' to mask xrdp, then re-run user_install.sh"
            echo ""
        fi

        # Enable and restart user gnome-remote-desktop
        systemctl --user enable gnome-remote-desktop 2>/dev/null || true
        systemctl --user restart gnome-remote-desktop 2>/dev/null || true

        # Wait and verify RDP is listening
        sleep 1
        if ss -tlnp 2>/dev/null | grep -q ":3389 " || netstat -tlnp 2>/dev/null | grep -q ":3389 "; then
            # Check if it's gnome-remote-desktop or xrdp
            if pgrep -x xrdp >/dev/null 2>&1; then
                echo "  ⚠ Port 3389 is xrdp (not gnome-remote-desktop)"
                echo "  Run 'sudo install.sh' to mask xrdp"
            else
                echo "  ✓ gnome-remote-desktop listening on port 3389"
                echo "  Connect with: rdp <IP> or xfreerdp3 /v:<IP>:3389 /u:$TARGET_USER"
            fi
        else
            echo "  ✗ gnome-remote-desktop NOT listening on port 3389"
            echo "  This usually means RDP credentials aren't set (keyring locked)"
            echo "  Fix keyring and re-run user_install.sh (see warning above)"
        fi
    else
        echo "  grdctl not found, skipping RDP configuration."
    fi
else
    echo "Skipping GNOME Remote Desktop (minimal install)..."
fi

# 9. Configure SSH Keys
echo "Configuring SSH keys..."
SSH_KEY="$USER_HOME/.ssh/id_ed25519"
if [ ! -f "$SSH_KEY" ]; then
    echo "  Generating SSH key at $SSH_KEY..."
    mkdir -p "$USER_HOME/.ssh"
    chmod 700 "$USER_HOME/.ssh"
    ssh-keygen -t ed25519 -f "$SSH_KEY" -N ""
    echo "  SSH key generated successfully."
else
    echo "  SSH key already exists at $SSH_KEY"
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
