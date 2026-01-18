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

echo "Starting Dashboard..." >> "$LOG_FILE"
date >> "$LOG_FILE"

# Start Bridge if not running
if is_running "node dashboard/bridge.cjs"; then
    echo "Bridge already running." >> "$LOG_FILE"
else
    echo "Starting Bridge..." >> "$LOG_FILE"
    # Ensure port 3501 is free
    fuser -k 3501/tcp >> "$LOG_FILE" 2>&1
    # Small delay to allow release
    sleep 1
    
    cd "$PROJECT_ROOT"
    nohup node dashboard/bridge.cjs >> "$LOG_FILE" 2>&1 &
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
