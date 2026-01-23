startLoomServerAndClient() {
    echo "Delegating to daemon service controller..."
    d restartLoom --dev
    echo "Loom services restarted via systemd. Check logs with: d log"
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

generateLoomToken() {
    d generateLoomToken
}

revokeLoomTokens() {
    d revokeLoomTokens
}
