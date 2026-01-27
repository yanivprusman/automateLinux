#!/bin/bash
# Comprehensive test suite for Chrome-Daemon communication

SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
LOG_FILE="/opt/automateLinux/data/combined.log"
TEST_MARKER="=== COMPREHENSIVE COMM TEST $(date +%s) ==="

echo "$TEST_MARKER" >> "$LOG_FILE"
echo "🧪 Starting Comprehensive Chrome-Daemon Communication Test"
echo ""

# Test 1: Check native host is running
echo "📍 Test 1: Native Host Status"
PID=$(ps aux | grep "automate-linux-native-host.py" | grep -v grep | awk '{print $2}' | head -1)
if [ -n "$PID" ]; then
    echo "  ✓ Native host running (PID: $PID)"
else
    echo "  ✗ Native host not running"
    exit 1
fi
echo ""

# Test 2: Send focus command
echo "📍 Test 2: Focus Command & ACK"
echo '{"command":"focusChatGPT"}' | nc -U "$SOCKET_PATH" -w 1 &
sleep 2

# Check for ACK in logs
ACK_FOUND=$(grep -A 5 "$TEST_MARKER" "$LOG_FILE" | grep -c "focusAck" || echo "0")
if [ "$ACK_FOUND" -gt 0 ]; then
    echo "  ✓ Focus ACK received"
    TIMEOUT=$(grep -A 10 "$TEST_MARKER" "$LOG_FILE" | grep -c "Focus ACK TIMEOUT")
    if [[ "$TIMEOUT" -gt 0 ]]; then
        echo "  ⚠ Warning: ACK timeout occurred"
    else
        echo "  ✓ No timeouts detected"
    fi
else
    echo "  ✗ No ACK found in logs"
fi
echo ""

# Test 3: Check message flow logging
echo "📍 Test 3: Message Flow Logging"
NATIVE_LOGS=$(grep -A 20 "$TEST_MARKER" "$LOG_FILE" | grep -c "\[NativeHost\]")
CHROME_LOGS=$(grep -A 20 "$TEST_MARKER" "$LOG_FILE" | grep -c "From Chrome")
echo "  Native host log entries: $NATIVE_LOGS"
echo "  Chrome message log entries: $CHROME_LOGS"
if [ "$NATIVE_LOGS" -gt 0 ] && [ "$CHROME_LOGS" -gt 0 ]; then
    echo "  ✓ Bidirectional logging working"
else
    echo "  ⚠ Limited logging detected"
fi
echo ""

# Test 4: Check for recent errors
echo "📍 Test 4: Error Detection"
ERRORS=$(grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -ci "error\|exception\|failed" || echo "0")
if [ "$ERRORS" -eq 0 ]; then
    echo "  ✓ No errors detected"
else
    echo "  ⚠ Found $ERRORS error(s) - check logs"
fi
echo ""

# Test 5: URL tracking
echo "📍 Test 5: URL Tracking"
RECENT_URL=$(grep "Active tab changed to:" "$LOG_FILE" | tail -1 | cut -d':' -f5- | cut -c 2-80)
if [ -n "$RECENT_URL" ]; then
    echo "  ✓ Last tracked URL: $RECENT_URL..."
else
    echo "  ⚠ No recent URL tracking"
fi
echo ""

# Summary
echo "═══════════════════════════════════════"
echo "📊 Test Summary"
echo "═══════════════════════════════════════"
grep -A 50 "$TEST_MARKER" "$LOG_FILE" | grep -E "\[NativeHost\].*→|← Chrome" | tail -10
echo ""
echo "✅ Communication test complete!"
echo "Check full logs at: $LOG_FILE"
