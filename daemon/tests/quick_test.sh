#!/bin/bash
# Quick test - sends "test" to the active window
SENDKEYS="/opt/automateLinux/utilities/sendKeys/sendKeys"
KEYBOARD_PATH="/dev/input/by-id/usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd"

echo "Quick test: Will type 'test' in 3 seconds..."
echo "Focus a text editor now!"
sleep 3
sudo $SENDKEYS -k "$KEYBOARD_PATH" keyT keyE keyS keyT
echo "Done! Did you see 'test' appear?"
