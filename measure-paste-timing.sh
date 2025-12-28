#!/bin/bash
# Precise timing measurement for Ctrl+V paste operation

SENDKEYS="/home/yaniv/coding/automateLinux/utilities/sendKeys/sendKeys"
LOG_FILE="/home/yaniv/coding/automateLinux/data/combined.log"

echo "=== CTRL+V TIMING TEST ==="
echo ""

# Clear old test marker
echo "TIMING_TEST_MARKER_$(date +%s)" >> "$LOG_FILE"

# Put unique test content in clipboard
TEST_CONTENT="TIMING_TEST_$(date +%s%3N)"
echo "$TEST_CONTENT" | xclip -selection clipboard
echo "Test content: $TEST_CONTENT"
echo ""

# Focus ChatGPT tab (Alt+1)
$SENDKEYS leftalt+1
sleep 0.3

# Start timing and execute Ctrl+V
echo "Executing Ctrl+V at $(date '+%H:%M:%S.%3N')..."
START_MS=$(date +%s%3N)

$SENDKEYS leftctrl+v

# Wait for operation to complete
sleep 1

END_MS=$(date +%s%3N)

# Extract timing from logs
echo ""
echo "=== LOG ANALYSIS ==="

# Get the last Ctrl+V sequence
LAST_SEQ=$(grep -A 10 "Ctrl+V detected" "$LOG_FILE" | tail -15)

if echo "$LAST_SEQ" | grep -q "Ctrl+V detected"; then
    echo "✅ Ctrl+V detected by daemon"
    
    # Check each step
    if echo "$LAST_SEQ" | grep -q "Triggering ChatGPT focus macro"; then
        echo "✅ Focus macro triggered"
        
        if echo "$LAST_SEQ" | grep -q "Sent focus request to native host"; then
            echo "✅ Focus request sent to extension"
            
            if echo "$LAST_SEQ" | grep -q "From Chrome.*focusAck"; then
                echo "✅ ACK received from extension"
                
                if echo "$LAST_SEQ" | grep -q "pasting NOW"; then
                    echo "✅ Paste executed"
                    
                    # Calculate timing
                    TOTAL_MS=$((END_MS - START_MS))
                    echo ""
                    echo "⏱️  TOTAL TIME: ${TOTAL_MS}ms"
                    echo ""
                    
                    if [ $TOTAL_MS -lt 150 ]; then
                        echo "🎉 EXCELLENT: Paste completed in <150ms"
                    elif [ $TOTAL_MS -lt 300 ]; then
                        echo "✅ GOOD: Paste completed in <300ms"  
                    else
                        echo "⚠️  SLOW: Paste took >${TOTAL_MS}ms (target <150ms)"
                    fi
                else
                    echo "❌ FAIL: Paste not executed"
                fi
            else
                echo "❌ FAIL: ACK not received (TIMEOUT)"
            fi
        else
            echo "❌ FAIL: Focus request not sent"
        fi
    else
        echo "❌ FAIL: Focus macro not triggered"
    fi
else
    echo "❌ FAIL: Ctrl+V not detected by daemon"
    echo "Possible causes:"
    echo "  - Wrong window focused"
    echo "  - Daemon not processing keyboard events"
    echo "  - Context not set to CHROME"
fi

echo ""
echo "Last 10 log lines:"
tail -10 "$LOG_FILE"
