#!/bin/bash
# Automate Chrome extension reload using simulated keypresses

SENDKEYS="/opt/automateLinux/utilities/sendKeys/sendKeys"

echo "Reloading Chrome extension..."

# Open new tab in Chrome
$SENDKEYS leftctrl+t
sleep 0.3

# Type chrome://extensions
$SENDKEYS "c h r o m e : / / e x t e n s i o n s"
sleep 0.2

# Press Enter to navigate
$SENDKEYS enter
sleep 0.5

# Press Ctrl+R to reload the first extension (Tab Tracker should be listed first)
$SENDKEYS leftctrl+r
sleep 0.3

# Close the extensions tab
$SENDKEYS leftctrl+w

echo "Extension reload complete!"
