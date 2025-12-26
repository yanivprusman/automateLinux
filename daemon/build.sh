(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping services..."
# Stop via safe script
/home/yaniv/coding/automateLinux/daemon/stop_daemon.sh


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
echo "Restarting daemon.service..." && \
sudo /usr/bin/systemctl restart daemon.service && \
sleep 0.1 && \
cd ..
