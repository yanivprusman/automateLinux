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
    # Launch auto-selector in background (DISABLED: causing popup loops)
    # (python3 ~/coding/automateLinux/utilities/autoSelectLoomScreen.py &) > /dev/null 2>&1
    sleep 3

    # 3. Handle cleanup on exit
    trap "echo 'Stopping Loom...'; kill $server_pid 2>/dev/null; fuser -k 4000/tcp 4100/tcp 2>/dev/null || true" EXIT INT TERM

    # 4. Start Client in foreground
    echo "Starting Loom Client on port 4000..."
    (cd ~/coding/loom/client && npm run dev)
}

forceLoomShare() {
    # 1. Find the window ID of "Share Screen"
    local win_id=$(d listWindows | grep -oP '(?<="id":)\d+(?=,"title":"Share Screen")' | head -n1)
    
    if [ -z "$win_id" ]; then
        echo "Error: 'Share Screen' window not found."
        return 1
    fi
    
    echo "Found Share Screen window: $win_id"
    
    # 2. Focus the window
    d activateWindow --windowId "$win_id"
    sleep 0.5
    
    # 3. Simulate Sequence: SPACE (Select) -> TAB -> TAB -> ENTER (Share)
    # Key codes: SPACE=57, TAB=15, ENTER=28
    
    # Send a nudge first (Shift)
    d simulateInput --type 1 --code 42 --value 1; sleep 0.05; d simulateInput --type 1 --code 42 --value 0
    sleep 0.2
    
    local keys=(57 15 15 28)
    
    for code in "${keys[@]}"; do
        echo "Pressing key code: $code"
        d simulateInput --type 1 --code "$code" --value 1
        sleep 0.1
        d simulateInput --type 1 --code "$code" --value 0
        sleep 0.3
    done
    
    echo "Done."
}
