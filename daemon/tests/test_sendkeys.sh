#!/bin/bash
# Test script for sendKeys functionality
# This will test various key commands using the standalone sendKeys utility

SENDKEYS="/opt/automateLinux/utilities/sendKeys/sendKeys"
KEYBOARD_PATH="/dev/input/by-id/usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd"

echo "=== SendKeys Test Suite ==="
echo "Keyboard: $KEYBOARD_PATH"
echo ""
echo "IMPORTANT: Please have a text editor or terminal focused to see the output!"
echo "Press Enter to start testing..."
read

echo ""
echo "Test 1: Single key press (keyA)"
echo "Expected: Letter 'a' should appear"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyA
echo "✓ Test 1 complete"
sleep 1

echo ""
echo "Test 2: Multiple keys (keyH keyE keyL keyL keyO)"
echo "Expected: Word 'hello' should appear"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyH keyE keyL keyL keyO
echo "✓ Test 2 complete"
sleep 1

echo ""
echo "Test 3: Space key"
echo "Expected: A space should appear"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" space
echo "✓ Test 3 complete"
sleep 1

echo ""
echo "Test 4: Shift key (what daemon uses)"
echo "Expected: Shift key press (may not be visible)"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyShift
echo "✓ Test 4 complete"
sleep 1

echo ""
echo "Test 5: Shift + A (using Down/Up)"
echo "Expected: Capital 'A' should appear"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyShiftDown keyA keyShiftUp
echo "✓ Test 5 complete"
sleep 1

echo ""
echo "Test 6: Enter key"
echo "Expected: New line"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" enter
echo "✓ Test 6 complete"
sleep 1

echo ""
echo "Test 7: Multiple shift presses (daemon use case)"
echo "Expected: Three shift key presses"
sleep 2
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyShift keyShift keyShift
echo "✓ Test 7 complete"

echo ""
echo "=== All tests complete ==="
echo "Did you see the expected output in your focused window? (y/n)"
read response

if [ "$response" = "y" ]; then
    echo "✓ SendKeys standalone tests PASSED"
    exit 0
else
    echo "✗ SendKeys standalone tests FAILED"
    exit 1
fi
