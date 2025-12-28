#!/bin/bash
# Automated test to verify focus ACK fix

SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"

echo "=== Starting Focus ACK Verification Test ==="
echo ""

# Test 1: Send focus command via socket
echo "Test: Sending focusChatGPT command to daemon..."
echo '{"command":"focusChatGPT"}' | nc -U "$SOCKET_PATH" -w 1 &

# Wait for processing
sleep 1

# Check recent logs for ACK
echo ""
echo "=== Last 10 Focus-related log entries ==="
grep -E "(Focus|focus|ACK|ack)" "$LOG_FILE" | tail -15
echo ""

# Analyze results
echo "=== Analysis ==="
RECENT_TIMEOUT=$(grep "Focus ACK TIMEOUT" "$LOG_FILE" | tail -5 | wc -l)
RECENT_SUCCESS=$(grep "Focus ACK confirmed" "$LOG_FILE" | tail -5 | wc -l)
RECENT_RECEIVED=$(grep "Focus ACK received" "$LOG_FILE" | tail -5 | wc -l)

echo "Recent ACK successes: $RECENT_SUCCESS"
echo "Recent ACK received: $RECENT_RECEIVED"
echo "Recent timeouts: $RECENT_TIMEOUT"
echo ""

if [ "$RECENT_TIMEOUT" -eq 0 ] && [ "$RECENT_RECEIVED" -gt 0 ]; then
    echo "✓ TEST PASSED: ACK received without timeout"
    exit 0
else
    echo "✗ TEST FAILED: Check logs for details"
    exit 1
fi
