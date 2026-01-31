#!/bin/bash
set -e

# 1. Cleanup
echo "Cleaning up..."
killall loom-server 2>/dev/null || true
pkill -f "vite" || true
rm -f ~/.config/loom/restore_token
sleep 5

# Restart portals to ensure clean state
echo "Restarting portal services..."
systemctl --user restart xdg-desktop-portal xdg-desktop-portal-gnome || true
sleep 5

# 2. Check Daemon
# echo "Checking daemon..."
# ./utilities/daemon-send-command/d ping

# 3. Start Server (Port 3500)
echo "Starting Loom Server..."
nohup ./extraApps/loom/server/build/loom-server 3500 > /tmp/loom_server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# 4. Start Auto-Selector
echo "Starting Auto-Selector..."
nohup python3 utilities/autoSelectLoomScreen.py > /tmp/loom_auto.log 2>&1 &

# 5. Start Client
echo "Starting Client..."
cd extraApps/loom/client
nohup npm run dev -- --host 0.0.0.0 --port 3005 > /tmp/loom_client.log 2>&1 &
CLIENT_PID=$!
echo "Client PID: $CLIENT_PID"

# Wait for client to be ready
echo "Waiting for client..."
timeout 30s bash -c "until curl -s http://localhost:3005 > /dev/null; do sleep 1; done"

echo "Loom stack currently running. Ready for browser test."
