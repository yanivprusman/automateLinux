(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping services..."

# Stop via systemd (proper way - ensures clean ungrab)
# Stop via systemd (proper way - ensures clean ungrab)
sudo /usr/bin/systemctl stop daemon.service 2>/dev/null || true
# Also kill any orphan processes not managed by systemd
./stop_daemon.sh 2>/dev/null || true
# Give time for ungrab to complete
sleep 0.5

# Ensure socket directory exists with correct permissions
if [ ! -d "/run/automatelinux" ]; then
    echo "Creating /run/automatelinux..."
    sudo /bin/mkdir -p /run/automatelinux
fi
# If running as root (e.g. during install), set to root:root. Otherwise use sudo user or current user.
if [ "$(id -u)" -eq 0 ]; then
    chown root:root /run/automatelinux
else
    sudo /usr/bin/chown $USER:$USER /run/automatelinux
fi

if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" && \
cp daemon .. && \
cd ..

if [ -z "$SKIP_SERVICE_RESTART" ]; then
    echo "Reloading systemd and starting daemon.service..."
    sudo /usr/bin/systemctl daemon-reload
    sudo /usr/bin/systemctl start daemon.service
fi
