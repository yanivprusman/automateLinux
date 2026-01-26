#!/bin/bash
set -e

INSTALL_DIR="/opt/automateLinux"
echo "Starting AutomateLinux System-Wide Installation..."

# 1. Require Root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)."
    exit 1
fi

echo "  Installation Directory: $INSTALL_DIR"

# 2. Error Handler
error_handler() {
    echo ""
    echo "--------------------------------------------------------"
    echo "Error: Installation failed at line $1"
    echo "The system may be in a partial state."
    echo "To revert changes, you can try stopping the service and removing $INSTALL_DIR:"
    echo "  sudo systemctl stop daemon.service"
    echo "  sudo rm /etc/systemd/system/daemon.service"
    echo "  sudo rm /usr/local/bin/daemon"
    echo "  sudo rm /etc/profile.d/automatelinux.sh"
    echo "--------------------------------------------------------"
    exit 1
}
trap 'error_handler $LINENO' ERR

# 1.5. Check and Install Dependencies
verify_dependencies() {
    # Add npm to required system packages
    echo "Checking build dependencies..."
    REQUIRED_PACKAGES="cmake make g++ libcurl4-openssl-dev pkg-config libmysqlcppconn-dev libboost-system-dev nlohmann-json3-dev libjsoncpp-dev libevdev-dev libsystemd-dev mysql-server git util-linux npm wireguard resolvconf"
    MISSING_PACKAGES=""

    for pkg in $REQUIRED_PACKAGES; do
        if ! dpkg -s $pkg >/dev/null 2>&1; then
            MISSING_PACKAGES="$MISSING_PACKAGES $pkg"
        fi
    done

    if [ -n "$MISSING_PACKAGES" ]; then
        echo "Missing packages: $MISSING_PACKAGES"
        echo "Installing missing dependencies..."
        apt-get update
        apt-get install -y $MISSING_PACKAGES
    else
        echo "All dependencies determined to be installed."
    fi
}
verify_dependencies

# 2.5. Install Extra Software
install_software() {
    echo "Checking for extra software..."
    
    # Check for @google/gemini-cli
    if ! command -v gemini >/dev/null 2>&1; then
        echo "  Installing @google/gemini-cli..."
        npm install -g @google/gemini-cli
    else
        echo "  @google/gemini-cli is already installed."
    fi
}
install_software

# 2.6. Configure Git Aliases
configure_git() {
    echo "Configuring system-wide git aliases..."
    git config --system alias.l "log --oneline -20"
    git config --system alias.r "reset --hard HEAD"
    git config --system alias.stat "status"
    git config --system alias.a "! git add -A && git status"
    git config --system alias.m "commit -m"
    git config --system alias.s "status"
    git config --system alias.alias "!git config --get-regexp alias"
}
configure_git


# 3. Enforce Installation Location
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ "$REPO_ROOT" != "$INSTALL_DIR" ]; then
    echo "Error: AutomateLinux must be cloned directly to $INSTALL_DIR"
    echo "Please run:"
    echo "  sudo git clone https://github.com/yanivprusman/automateLinux.git $INSTALL_DIR"
    echo "  cd $INSTALL_DIR"
    echo "  sudo ./install.sh"
    exit 1
fi
echo "Running in-place at $INSTALL_DIR."

# 4. Create data directories and set permissions
echo "Configuring permissions..."
mkdir -p "$INSTALL_DIR/data"
mkdir -p "$INSTALL_DIR/config"
mkdir -p "$INSTALL_DIR/symlinks"

# Set ownership: root for system service, coding group for developer access
if ! getent group coding >/dev/null 2>&1; then
    echo "  'coding' group not found. Creating it..."
    groupadd coding
fi

echo "  Applied root:coding ownership with group write access."
chown -R root:coding "$INSTALL_DIR"
chmod -R g+w "$INSTALL_DIR"

# Add current sudo user to coding group if applicable
if [ -n "$SUDO_USER" ]; then
    if ! id -nG "$SUDO_USER" | grep -qw "coding"; then
        echo "  Adding user $SUDO_USER to 'coding' group..."
        usermod -aG coding "$SUDO_USER"
        echo "  Note: You may need to log out and back in for group changes to take full effect."
    fi
fi

# Ensure data and config are group-writable for the daemon/scripts
chmod -R 775 "$INSTALL_DIR/data"
chmod -R 775 "$INSTALL_DIR/config"

# 5. Build Daemon
echo "Building Daemon at $INSTALL_DIR/daemon..."
if [ -f "$INSTALL_DIR/daemon/build.sh" ]; then
    pushd "$INSTALL_DIR/daemon" > /dev/null
    # We are root, so build.sh runs as root.
    # Prevent build.sh from trying to restart the service (it doesn't exist yet)
    export SKIP_SERVICE_RESTART=true
    source ./build.sh
    popd > /dev/null
else
    echo "Error: build.sh not found in target directory."
    exit 1
fi

# 6. Symlink 'd' command and helpers
echo "Updating symlinks..."
ln -sf "../daemon/daemon" "$INSTALL_DIR/symlinks/daemon"
ln -sf "../terminal/theRealPath.sh" "$INSTALL_DIR/symlinks/theRealPath"
ln -sf "$INSTALL_DIR/symlinks/daemon" /usr/local/bin/daemon

# 7. Install Systemd Service (Root Service)
echo "Installing Systemd Service (root)..."
SERVICE_FILE_CONTENT="[Unit]
Description=Automate Daemon (System)
After=network.target

[Service]
Type=simple
ExecStart=/bin/bash -c '$INSTALL_DIR/daemon/daemon daemon 2>&1 | tee -a $INSTALL_DIR/data/combined.log'
Restart=on-failure
RestartSec=2
User=root
Group=root
Environment=AUTOMATE_LINUX_DIR=$INSTALL_DIR
StandardOutput=journal
StandardError=journal
RuntimeDirectory=automatelinux
RuntimeDirectoryMode=0750

[Install]
WantedBy=multi-user.target
"

echo "$SERVICE_FILE_CONTENT" > /etc/systemd/system/daemon.service
systemctl daemon-reload
systemctl enable daemon.service
systemctl restart daemon.service

# 8. Configure System-Wide PATH
echo "Configuring system-wide PATH..."
PROFILE_SCRIPT="/etc/profile.d/automatelinux.sh"
echo 'export PATH=$PATH:/opt/automateLinux/symlinks' > "$PROFILE_SCRIPT"
chmod 644 "$PROFILE_SCRIPT"

echo "--------------------------------------------------------"
echo "System-Wide Installation Complete!"
echo "Installed to: $INSTALL_DIR"
echo "Daemon running as: root"
echo ""
echo "To configure for a user, run: sh $INSTALL_DIR/user_install.sh"
echo "--------------------------------------------------------"
