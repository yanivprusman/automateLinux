(return 0 2>/dev/null) || { echo "Script must be sourced"; exit 1; }
echo "Stopping services..."
sudo systemctl stop automateLinux.service || true
sudo systemctl stop daemon.service || true

# Aggressively kill any matching daemon processes
echo "Checking for running daemon processes..."
# Kill any process that looks like our daemon
sudo pkill -9 -f "daemon/daemon" || true
sudo pkill -9 -f "daemon daemon" || true

# Wait loop to ensure they are gone
MAX_RETRIES=10
COUNT=0
while pgrep -f "daemon/daemon" > /dev/null || pgrep -f "daemon daemon" > /dev/null; do
    echo "Waiting for daemon processes to exit..."
    sudo pkill -9 -f "daemon/daemon" || true
    sudo pkill -9 -f "daemon daemon" || true
    sleep 0.5
    COUNT=$((COUNT+1))
    if [ $COUNT -ge $MAX_RETRIES ]; then
        echo "WARNING: Failed to kill all daemon processes after 5 seconds."
        # Attempt to show what's still running for debugging
        ps aux | grep daemon | grep -v grep
        break
    fi
done

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
