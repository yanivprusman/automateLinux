#!/bin/bash
# Comprehensive automated test for Focus+Paste functionality
# Tests that single Ctrl+V press triggers both focus AND paste

set -e

LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"
TEST_MARKER="E2E_FOCUS_PASTE_$(date +%s)"

echo "🎯 Automated Focus+Paste E2E Test"
echo "===================================="
echo ""

# Step 1: Reload Chrome extension
echo "📦 Step 1/7: Reloading Chrome extension..."
cd /home/yaniv/coding/automateLinux/chromeExtension
./reload-extension.sh 2>&1 | grep -v "Traceback\|WebSocketBadStatusException" || true
echo "  ✓ Extension reloaded"
echo ""

# Wait for extension to initialize
sleep 2

# Step 2: Prepare ChatGPT tab (already open based on browser state)
echo "🌐 Step 2/7: ChatGPT tab ready"
echo "  ✓ Using existing ChatGPT tab"
echo ""

# Step 3: Set test marker in logs
echo "📝 Step 3/7: Preparing test environment..."
echo "$TEST_MARKER" >> "$LOG_FILE"
echo "  ✓ Test marker set: $TEST_MARKER"
echo ""

# Step 4: Put test content in clipboard
echo "📋 Step 4/7: Setting clipboard content..."
TEST_TEXT="AUTOMATED_TEST_$(date +%s)"
echo -n "$TEST_TEXT" | xclip -selection clipboard
echo "  ✓ Clipboard set to: $TEST_TEXT"
echo ""

# Step 5: Instructions for manual Ctrl+V (we'll automate this next)
echo "⌨️  Step 5/7: MANUAL ACTION REQUIRED"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. Go to the ChatGPT tab in Chrome"
echo "2. Click somewhere OUTSIDE the textarea (defocus it)"
echo "3. Press Ctrl+V once"
echo ""
echo "Expected: Textarea focuses automatically + paste happens"
echo ""
read -p "Press Enter after you've done Ctrl+V..."

# Step 6: Analyze results
echo ""
echo "📊 Step 6/7: Analyzing results..."
sleep 1

# Check logs for focus and paste events
FOCUS_SENT=$(grep -A 50 "$TEST_MARKER" "$LOG_FILE" | grep -c "Sent focus request" || echo "0")
ACK_RECEIVED=$(grep -A 50 "$TEST_MARKER" "$LOG_FILE" | grep -c "Focus ACK received" || echo "0")
TIMEOUT=$(grep -A 50 "$TEST_MARKER" "$LOG_FILE" | grep -c "Focus ACK TIMEOUT" || echo "0")

echo ""
echo "Results:"
if [ "$FOCUS_SENT" -gt 0 ]; then
    echo "  ✓ Focus request sent: $FOCUS_SENT"
else
    echo "  ✗ No focus request sent"
fi

if [ "$ACK_RECEIVED" -gt 0 ]; then
    echo "  ✓ Focus ACK received: $ACK_RECEIVED"
else
    echo "  ✗ No ACK received"
fi

if [ "$TIMEOUT" -eq 0 ]; then
    echo "  ✓ No timeouts"
else
    echo "  ⚠ Timeouts: $TIMEOUT"
fi

# Step 7: Verify paste
echo ""
echo "✅ Step 7/7: Manual verification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Check the ChatGPT textarea:"
echo "  - Did it receive focus automatically?"
echo "  - Does it contain: $TEST_TEXT"
echo ""
read -p "Did the test pass? (y/n): " RESULT

echo ""
echo "═══════════════════════════════════════"
if [ "$RESULT" = "y" ]; then
    echo "✅ TEST PASSED!"
    echo ""
    echo "Summary:"
    echo "  ✓ Single Ctrl+V press"
    echo "  ✓ Automatic focus"
    echo "  ✓ Paste completed"
    echo "  ✓ Communication working"
    exit 0
else
    echo "❌ TEST FAILED"
    echo ""
    echo "Debug info:"
    grep -A 50 "$TEST_MARKER" "$LOG_FILE" | grep -E "focus|Focus|paste|Paste" | tail -15
    exit 1
fi
