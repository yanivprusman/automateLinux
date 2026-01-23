#!/bin/bash
set -ne

INSTALL_DIR="/opt/automateLinux"
echo "Starting AutomateLinux System-Wide Installation..."

# 1. Require Root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)."
    exit 1
fi

echo "  Installation Directory: $INSTALL_DIR"

# 1.5. Check and Install Dependencies
verify_dependencies() {
    echo "Checking build dependencies..."
    REQUIRED_PACKAGES="cmake make g++ libcurl4-openssl-dev pkg-config libmysqlcppconn-dev libboost-system-dev libjsoncpp-dev libevdev-dev libsystemd-dev mysql-server git"
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

# 2. Prepare Installation Directory
if [ -d "$INSTALL_DIR" ]; then
    echo "Cleaning up existing installation at $INSTALL_DIR..."
    rm -rf "$INSTALL_DIR"
fi
mkdir -p "$INSTALL_DIR"

# 3. Copy Files (Assuming script runs from repo root)
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Copying files from $REPO_ROOT to $INSTALL_DIR..."
cp -r "$REPO_ROOT"/. "$INSTALL_DIR"/

# 4. Create data directories and set permissions
echo "Configuring permissions..."
mkdir -p "$INSTALL_DIR/data"
mkdir -p "$INSTALL_DIR/symlinks"

# Set ownership:
# Everything -> root:root
chown -R root:root "$INSTALL_DIR"
chmod -R 755 "$INSTALL_DIR"

# 5. Build Daemon
echo "Building Daemon at $INSTALL_DIR/daemon..."
if [ -f "$INSTALL_DIR/daemon/build.sh" ]; then
    pushd "$INSTALL_DIR/daemon" > /dev/null
    # We are root, so build.sh runs as root.
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

echo "--------------------------------------------------------"
echo "System-Wide Installation Complete!"
echo "Installed to: $INSTALL_DIR"
echo "Daemon running as: root"
echo ""
echo "To configure for a user, run: sh $INSTALL_DIR/user_install.sh"
echo "--------------------------------------------------------"
