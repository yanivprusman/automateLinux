(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping automateLinux.service..."
sudo systemctl stop automateLinux.service || true
sudo pkill -9 -f "daemon daemon" || true

if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" && \
echo "Starting automateLinux.service..." && \
sudo systemctl start automateLinux.service && \
sleep 1 && \
cd ..
