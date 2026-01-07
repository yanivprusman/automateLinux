startLoomServerAndClient() {
    # 1. Cleanup existing Loom processes
    echo "Cleaning up Loom processes and ports (4000-4005, 4100)..."
    killall loom-server 2>/dev/null || true
    # Clear a range of ports to avoid Vite "port drifting"
    fuser -k 4000/tcp 4001/tcp 4002/tcp 4003/tcp 4004/tcp 4005/tcp 4100/tcp 2>/dev/null || true

    # 2. Start Server in background
    echo "Starting Loom Server on port 4100..."
    (cd ~/coding/loom/server/build && ./loom-server 4100) &
    local server_pid=$!

    echo "Wait for the Screen Share popup on your PC and click 'Share'..."
    sleep 3

    # 3. Handle cleanup on exit
    trap "echo 'Stopping Loom...'; kill $server_pid 2>/dev/null; fuser -k 4000/tcp 4100/tcp 2>/dev/null || true" EXIT INT TERM

    # 4. Start Client in foreground
    echo "Starting Loom Client on port 4000..."
    (cd ~/coding/loom/client && npm run dev)
}
