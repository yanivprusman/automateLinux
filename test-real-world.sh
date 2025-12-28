#!/bin/bash
# Real-world test: Verify focus+paste works via actual Ctrl+V keyboard trigger
# This simulates exactly how the user would use it

LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"
TEST_MARKER="REAL_WORLD_TEST_$(date +%s)"

echo "🎯 Real-World Focus+Paste Test"
echo "==============================="
echo ""

# Pre-flight checks
echo "📋 Checking system status..."
if ! pgrep -f "chrome" > /dev/null; then
    echo "✗ Chrome not running"
    exit 1
fi
echo "✓ Chrome running"

if ! pgrep -f "automateLinux" > /dev/null; then
    echo "✗ Daemon not running"
    exit 1
fi
echo "✓ Daemon running"

NATIVE_PID=$(pgrep -f "automate-linux-native-host.py" | head -1)
if [ -z "$NATIVE_PID" ]; then
    echo "✗ Native host not running"
    exit 1
fi
echo "✓ Native host running (PID: $NATIVE_PID)"
echo ""

# Check if we have the new code
echo "🔍 Checking native host version..."
if grep -q "\[NativeHost\]" /usr/local/bin/automate-linux-native-host.py 2>/dev/null; then
    echo "✓ Enhanced native host deployed"
else
    echo "⚠ Old native host - run:"
    echo "  sudo cp /home/yaniv/coding/automateLinux/chromeExtension/native-host.py \\"
    echo "          /usr/local/bin/automate-linux-native-host.py"
    echo "  pkill -f automate-linux-native-host.py"
    echo ""
fi

# Mark logs
echo "$TEST_MARKER" >> "$LOG_FILE"

echo "🧪 Testing Instructions:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. Make sure a ChatGPT tab is open in Chrome"
echo "2. Click somewhere OUTSIDE the textarea (to defocus it)"
echo "3. Press Ctrl+V"
echo ""
echo "Expected result:"
echo "  → Textarea should receive focus automatically"
echo "  → Paste should occur"
echo "  → Total time should be <200ms"
echo ""
read -p "Press Enter when ready to start monitoring, then do the test..."

echo ""
echo "⏱️  Monitoring logs (press Ctrl+C when done)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Monitor logs in real-time
tail -f "$LOG_FILE" | grep --line-buffered -E "focus|Focus|ACK|ack|NativeHost.*→|NativeHost.*←" &
TAIL_PID=$!

# Wait for user to complete test
echo ""
echo "Waiting for test completion (Ctrl+C to stop)..."
wait $TAIL_PID 2>/dev/null || true

echo ""
echo "📊 Test Results:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Analyze what happened after the marker
ACK_COUNT=$(grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -c "focusAck" || echo "0")
TIMEOUT_COUNT=$(grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -c "Focus ACK TIMEOUT" || echo "0")
FOCUS_REQUEST=$(grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -c "Sent focus request" || echo "0")

if [ "$FOCUS_REQUEST" -gt 0 ]; then
    echo "✓ Focus request sent: $FOCUS_REQUEST time(s)"
else
    echo "✗ No focus request found - did you press Ctrl+V in Chrome?"
fi

if [ "$ACK_COUNT" -gt 0 ]; then
    echo "✓ Focus ACK received: $ACK_COUNT time(s)"
    
    if [ "$TIMEOUT_COUNT" -eq 0 ]; then
        echo "✓ No timeouts - ACK received quickly!"
    else
        echo "⚠ Timeouts occurred: $TIMEOUT_COUNT"
    fi
else
    echo "⚠ No ACK found in logs"
fi

echo ""
echo "Recent log entries:"
grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -E "focus|Focus|ACK|NativeHost" | tail -10

echo ""
read -p "Did the test work? (y/n): " SUCCESS

if [ "$SUCCESS" = "y" ]; then
    echo ""
    echo "✅ TEST PASSED!"
    echo "   Focus+paste automation is working perfectly!"
    exit 0
else
    echo ""
    echo "❌ TEST FAILED"
    echo "   Check logs around marker: $TEST_MARKER"
    exit 1
fi
