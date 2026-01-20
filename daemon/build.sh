(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping services..."

# Stop via systemd (proper way - ensures clean ungrab)
sudo /usr/bin/systemctl stop daemon.service 2>/dev/null || true
# Also kill any orphan processes not managed by systemd
/home/yaniv/coding/automateLinux/daemon/stop_daemon.sh 2>/dev/null || true
# Give time for ungrab to complete
sleep 0.5

# Ensure socket directory exists with correct permissions
if [ ! -d "/run/automatelinux" ]; then
    echo "Creating /run/automatelinux..."
    sudo /bin/mkdir -p /run/automatelinux
fi
sudo /usr/bin/chown $USER:$USER /run/automatelinux

if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" && \
cp daemon .. && \
echo "Reloading systemd and starting daemon.service..." && \
sudo /usr/bin/systemctl daemon-reload && \
sudo /usr/bin/systemctl start daemon.service && \
sleep 0.3 && \
cd ..
