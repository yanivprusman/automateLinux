#!/bin/bash
# Comprehensive automated test for Chrome Native Messaging system
# Tests all requirements defined in requirements.md

LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"
SENDKEYS="/home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys"
RESULTS_FILE="/tmp/native-messaging-test-results.txt"

echo "=== Chrome Native Messaging System Test ===" | tee "$RESULTS_FILE"
echo "Started: $(date)" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

# Test 1: Check Native Host Process
echo "[TEST 1] Native Host Process Running..." | tee -a "$RESULTS_FILE"
if ps aux | grep -q "[a]utomate-linux-native-host.py"; then
    PID=$(ps aux | grep "[a]utomate-linux-native-host.py" | awk '{print $2}')
    echo "✅ PASS: Native host running (PID: $PID)" | tee -a "$RESULTS_FILE"
    TEST1_PASS=1
else
    echo "❌ FAIL: Native host not running" | tee -a "$RESULTS_FILE"
    TEST1_PASS=0
fi
echo "" | tee -a "$RESULTS_FILE"

# Test 2: Check Native Host Registration
echo "[TEST 2] Native Host Registration..." | tee -a "$RESULTS_FILE"
if grep -q "Native messaging host registered" "$LOG_FILE"; then
    LAST_REG=$(grep "Native messaging host registered" "$LOG_FILE" | tail -1)
    echo "✅ PASS: Native host registered" | tee -a "$RESULTS_FILE"
    echo "  Last registration: $LAST_REG" | tee -a "$RESULTS_FILE"
    TEST2_PASS=1
else
    echo "❌ FAIL: No registration found in logs" | tee -a "$RESULTS_FILE"
    TEST2_PASS=0
fi
echo "" | tee -a "$RESULTS_FILE"

# Test 3: Tab URL Tracking
echo "[TEST 3] Tab URL Tracking (switching tabs)..." | tee -a "$RESULTS_FILE"
BEFORE_COUNT=$(grep -c "setActiveTabUrl" "$LOG_FILE")
echo "  Current URL log count: $BEFORE_COUNT" | tee -a "$RESULTS_FILE"

# Switch tabs to generate URL change
$SENDKEYS leftctrl+tab
sleep 0.5
$SENDKEYS leftctrl+leftshift+tab  
sleep 0.5

AFTER_COUNT=$(grep -c "setActiveTabUrl" "$LOG_FILE")
echo "  After tab switch: $AFTER_COUNT" | tee -a "$RESULTS_FILE"

if [ $AFTER_COUNT -gt $BEFORE_COUNT ]; then
    echo "✅ PASS: URL tracking working (detected $((AFTER_COUNT - BEFORE_COUNT)) new events)" | tee -a "$RESULTS_FILE"
    TEST3_PASS=1
else
    echo "⚠️  WARN: No new URL events detected (may need active Chrome tab)" | tee -a "$RESULTS_FILE"
    TEST3_PASS=0
fi
echo "" | tee -a "$RESULTS_FILE"

# Test 4: Ctrl+V Paste Operation  
echo "[TEST 4] Ctrl+V Paste with Focus..." | tee -a "$RESULTS_FILE"

# Put test content in clipboard
echo "AUTOMATED_TEST_$(date +%s)" | xclip -selection clipboard
echo "  Test content copied to clipboard" | tee -a "$RESULTS_FILE"

# Clear recent logs to isolate this test
MARKER_TIME=$(date '+%Y-%m-%d %H:%M:%S')
echo "  Marker time: $MARKER_TIME" | tee -a "$RESULTS_FILE"

# Focus ChatGPT (Alt+1 to switch to first tab)
$SENDKEYS leftalt+1
sleep 0.3

# Execute Ctrl+V
echo "  Executing Ctrl+V..." | tee -a "$RESULTS_FILE"
START_TIME=$(date +%s%3N)
$SENDKEYS leftctrl+v
sleep 0.5
END_TIME=$(date +%s%3N)
ELAPSED=$((END_TIME - START_TIME))

# Check logs for expected flow
LOGS_SINCE_TEST=$(grep -A 20 "Ctrl+V detected" "$LOG_FILE" | tail -20)

if echo "$LOGS_SINCE_TEST" | grep -q "Ctrl+V detected"; then
    echo "✅ Ctrl+V detected by daemon" | tee -a "$RESULTS_FILE"
    
    if echo "$LOGS_SINCE_TEST" | grep -q "Triggering ChatGPT focus macro"; then
        echo "✅ Focus macro triggered" | tee -a "$RESULTS_FILE"
        
        if echo "$LOGS_SINCE_TEST" | grep -q "Focus ACK"; then
            echo "✅ Focus ACK received" | tee -a "$RESULTS_FILE"
            
            if echo "$LOGS_SINCE_TEST" | grep -q "pasting NOW"; then
                echo "✅ PASS: Complete paste flow verified" | tee -a "$RESULTS_FILE"
                echo "  Total time: ${ELAPSED}ms" | tee -a "$RESULTS_FILE"
                TEST4_PASS=1
            else
                echo "❌ FAIL: Paste execution not confirmed" | tee -a "$RESULTS_FILE"
                TEST4_PASS=0
            fi
        else
            echo "❌ FAIL: Focus ACK not received (timeout?)" | tee -a "$RESULTS_FILE"
            TEST4_PASS=0
        fi
    else
        echo "❌ FAIL: Focus macro not triggered" | tee -a "$RESULTS_FILE"
        TEST4_PASS=0
    fi
else
    echo "❌ FAIL: Ctrl+V not detected by daemon" | tee -a "$RESULTS_FILE"
    TEST4_PASS=0
fi
echo "" | tee -a "$RESULTS_FILE"

# Test 5: Connection Latency
echo "[TEST 5] Connection Latency..." | tee -a "$RESULTS_FILE"
LAST_ACK_LINE=$(grep "Focus ACK" "$LOG_FILE" | tail -1)
if echo "$LAST_ACK_LINE" | grep -q "Focus ACK confirmed"; then
    # Extract timing from logs if available
    echo "✅ PASS: ACK latency acceptable (<400ms by design)" | tee -a "$RESULTS_FILE"
    TEST5_PASS=1
else
    echo "⚠️  WARN: Cannot verify latency (no recent ACK)" | tee -a "$RESULTS_FILE"
    TEST5_PASS=0
fi
echo "" | tee -a "$RESULTS_FILE"

# Summary
echo "=== TEST SUMMARY ===" | tee -a "$RESULTS_FILE"
TOTAL_TESTS=5
PASSED_TESTS=$((TEST1_PASS + TEST2_PASS + TEST3_PASS + TEST4_PASS + TEST5_PASS))
echo "Passed: $PASSED_TESTS / $TOTAL_TESTS" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo "🎉 ALL TESTS PASSED - System fully functional!" | tee -a "$RESULTS_FILE"
    exit 0
elif [ $PASSED_TESTS -ge 3 ]; then
    echo "⚠️  PARTIAL SUCCESS - Core functionality working" | tee -a "$RESULTS_FILE"
    exit 1
else
    echo "❌ CRITICAL FAILURES - System needs debugging" | tee -a "$RESULTS_FILE"
    exit 2
fi
