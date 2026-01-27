#!/bin/bash
# Test daemon's sendKeys integration by triggering activeWindowChanged
# This simulates what happens when you switch to VS Code

echo "=== Testing Daemon SendKeys Integration ==="
echo ""

# Enable logging
echo "1. Enabling daemon logging..."
daemon send shouldLog --enable true
echo ""

# Check current keyboard path
echo "2. Checking keyboard path..."
KEYBOARD_PATH=$(daemon send getEntry --key keyboardPath)
echo "   Keyboard path: $KEYBOARD_PATH"
echo ""

# Test by sending activeWindowChanged command with Code wmClass
echo "3. Simulating window change to Code (should send 3x keyShift)..."
echo "   Focus a text editor or terminal to see if shift keys are sent!"
sleep 3

# Send the activeWindowChanged command
echo '{"command":"activeWindowChanged","windowTitle":"test","wmClass":"Code","wmInstance":"code","windowId":12345}' | nc -U /run/automatelinux/automatelinux-daemon.sock

echo ""
echo "4. Checking daemon logs..."
sleep 1
tail -20 /opt/automateLinux/data/daemon.log | grep -A 2 "ACTIVE_WINDOW_CHANGED\|keyShift"

echo ""
echo "=== Test Complete ==="
echo "Did you see any shift key activity? Check the logs above."
