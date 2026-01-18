#!/bin/bash

# Define paths
DAEMON_DIR="$(dirname "$(dirname "$(readlink -f "$0")")")"
PROJECT_ROOT="$(dirname "$DAEMON_DIR")"
DASHBOARD_DIR="$PROJECT_ROOT/dashboard"
LOG_FILE="/tmp/automatelinux-dashboard.log"

# Function to check if a process is running
is_running() {
    pgrep -f "$1" > /dev/null
}

# Function to check if the bridge port is listening
is_bridge_listening() {
    # Check for port 3501 using ss or netstat depending on availability
    if command -v ss >/dev/null 2>&1; then
        ss -tlnH | grep -q ":3501"
    else
        netstat -tln | grep -q ":3501 "
    fi
}

echo "Starting Dashboard..." >> "$LOG_FILE"
date >> "$LOG_FILE"

# Start Bridge
# We use port check because process check is unreliable (process might exist but be zombie or not listening)
if is_bridge_listening; then
    echo "Bridge is listening on port 3501." >> "$LOG_FILE"
else
    echo "Bridge not listening on port 3501. Attempting start..." >> "$LOG_FILE"
    
    # Kill any stale processes matches
    pkill -f "node dashboard/bridge.cjs"
    
    # Ensure port 3501 is free
    fuser -k 3501/tcp >> "$LOG_FILE" 2>&1
    
    # Small delay
    sleep 1
    
    cd "$PROJECT_ROOT"
    nohup node dashboard/bridge.cjs >> "$LOG_FILE" 2>&1 &
    
    # Verify startup
    sleep 2
    if is_bridge_listening; then
        echo "Bridge started successfully." >> "$LOG_FILE"
    else
        echo "ERROR: Bridge failed to start or bind to port." >> "$LOG_FILE"
        # Log why it failed if possible?
        # The output of bridge.cjs is in LOG_FILE, so user should check there.
    fi
fi

# Start Frontend if not running
if is_running "vite"; then
    echo "Frontend already running." >> "$LOG_FILE"
else
    echo "Starting Frontend..." >> "$LOG_FILE"
    cd "$DASHBOARD_DIR"
    nohup npm run dev -- --port 3007 --host >> "$LOG_FILE" 2>&1 &
fi

echo "Dashboard startup check complete." >> "$LOG_FILE"
