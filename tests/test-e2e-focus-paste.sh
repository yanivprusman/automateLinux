#!/bin/bash
# End-to-end automated test for Chrome focus + paste functionality
# This test validates the complete flow: defocus → trigger paste → focus → paste succeeds

set -e

LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"
SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
TEST_MARKER="E2E_TEST_$(date +%s)"

echo "🎯 End-to-End Chrome Focus+Paste Test"
echo "======================================="
echo ""

# Step 1: Deploy native host if needed
echo "📦 Step 1: Checking native host deployment..."
if ! diff -q /usr/local/bin/automate-linux-native-host.py /home/yaniv/coding/automateLinux/chromeExtension/native-host.py >/dev/null 2>&1; then
    echo "  ⚠  Native host needs update"
    echo "  Run: sudo cp /home/yaniv/coding/automateLinux/chromeExtension/native-host.py /usr/local/bin/automate-linux-native-host.py"
    echo "  Then: pkill -f automate-linux-native-host.py"
    echo ""
    read -p "Press Enter after updating native host..."
fi
echo "  ✓ Native host ready"
echo ""

# Step 2: Check prerequisites
echo "📋 Step 2: Checking prerequisites..."
if ! pgrep -f "automate-linux-native-host.py" > /dev/null; then
    echo "  ✗ Native host not running!"
    exit 1
fi
echo "  ✓ Native host running (PID: $(pgrep -f automate-linux-native-host.py))"

if ! pgrep -f "automateLinux" > /dev/null; then
    echo "  ✗ Daemon not running!"
    exit 1  
fi
echo "  ✓ Daemon running"

if ! pgrep -f "chrome" > /dev/null; then
    echo "  ✗ Chrome not running!"
    exit 1
fi
echo "  ✓ Chrome running"
echo ""

# Step 3: Setup test marker
echo "📝 Step 3: Preparing test..."
echo "$TEST_MARKER" >> "$LOG_FILE"
sleep 1
echo ""

# Step 4: Open ChatGPT and defocus
echo "🌐 Step 4: Opening ChatGPT..."
echo "Manual steps:"
echo "  1. Open https://chatgpt.com in Chrome"
echo "  2. Wait for page to load"
echo "  3. Click somewhere OUTSIDE the textarea to defocus it"
echo "  4. Press Enter here when ready..."
read -p ""
echo "  ✓ ChatGPT ready and defocused"
echo ""

# Step 5: Trigger focus+paste via daemon
echo "⌨️  Step 5: Triggering focus+paste..."
START_TIME=$(date +%s%3N)
echo '{"command":"focusChatGPT"}' | nc -U "$SOCKET_PATH" -w 1 > /dev/null 2>&1 &
sleep 2
END_TIME=$(date +%s%3N)
ELAPSED=$((END_TIME - START_TIME))
echo "  ⏱  Elapsed: ${ELAPSED}ms"
echo ""

# Step 6: Analyze results
echo "📊 Step 6: Analyzing results..."
echo ""

# Check for ACK
ACK_FOUND=$(grep -A 20 "$TEST_MARKER" "$LOG_FILE" | grep -c "focusAck" || echo "0")
if [ "$ACK_FOUND" -gt 0 ]; then
    echo "  ✓ Focus ACK received"
    
    # Check ACK details
    ACK_DETAILS=$(grep -A 20 "$TEST_MARKER" "$LOG_FILE" | grep "focusAck" | tail -1)
    echo "    Details: $ACK_DETAILS"
    
    # Check for timeout
    if grep -A 20 "$TEST_MARKER" "$LOG_FILE" | grep -q "Focus ACK TIMEOUT"; then
        echo "  ✗ ACK timeout occurred!"
    else
        echo "  ✓ No timeout - ACK received quickly"
    fi
else
    echo "  ✗ No ACK found!"
fi

# Check for errors
ERROR_COUNT=$(grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -ci "error\|exception" || echo "0")
if [ "$ERROR_COUNT" -eq 0 ]; then
    echo "  ✓ No errors detected"
else
    echo "  ⚠  Found $ERROR_COUNT error(s)"
fi

# Show timing breakdown
echo ""
echo "  ⏱  Timing breakdown:"
grep -A 30 "$TEST_MARKER" "$LOG_FILE" | grep -E "NativeHost|Focus ACK|elapsed" | tail -5

echo ""
echo "🎯 Manual Verification Required:"
echo "  ❓ Did the textarea receive focus?"
echo "  ❓ Did paste occur successfully?"
echo ""
read -p "Was the test successful? (y/n): " SUCCESS

if [ "$SUCCESS" = "y" ]; then
    echo ""
    echo "✅ TEST PASSED!"
    echo "   Chrome focus+paste automation is working perfectly"
    exit 0
else
    echo ""
    echo "❌ TEST FAILED"
    echo "   Check logs at: $LOG_FILE"
    echo "   Look for errors around marker: $TEST_MARKER"
    exit 1
fi
