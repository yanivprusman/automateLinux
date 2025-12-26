(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping services..."
sudo systemctl stop daemon.service || true

# Stop via safe script
/home/yaniv/coding/automateLinux/daemon/stop_daemon.sh


# Ensure socket directory exists with correct permissions
if [ ! -d "/run/automatelinux" ]; then
    echo "Creating /run/automatelinux..."
    sudo mkdir -p /run/automatelinux
fi
sudo chown $USER:$USER /run/automatelinux

if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" && \
echo "Starting daemon.service..." && \
sudo systemctl start daemon.service && \
sleep 0.1 && \
cd ..
