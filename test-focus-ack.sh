#!/bin/bash
# Test script to trigger focus command multiple times and analyze ACK timing

SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"

echo "Starting focus ACK timing test..."
echo "Clearing log excerpt..."
echo "=== TEST START $(date) ===" >> "$LOG_FILE"

# Send focus command 10 times with 2 second delays
for i in {1..10}; do
    echo "Test $i: Sending focusChatGPT command..."
    echo '{"command":"focusChatGPT"}' | nc -U "$SOCKET_PATH"
    sleep 2
done

echo "Test complete. Analyzing results..."
echo ""
echo "=== RESULTS ==="
grep -E "(Focus ACK|Sent focus request)" "$LOG_FILE" | tail -30
echo ""
echo "=== SUMMARY ==="
SUCCESS=$(grep "Focus ACK confirmed via CV" "$LOG_FILE" | tail -10 | wc -l)
TIMEOUT=$(grep "Focus ACK TIMEOUT" "$LOG_FILE" | tail -10 | wc -l)
echo "Successes: $SUCCESS"
echo "Timeouts: $TIMEOUT"
echo "Success rate: $(echo "scale=2; $SUCCESS * 100 / ($SUCCESS + $TIMEOUT)" | bc)%"
