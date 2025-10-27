#!/bin/bash

# Check and open VS Code
if ! pgrep -x "code" > /dev/null; then
    echo "Opening VS Code..."
    code &
else
    echo "VS Code is already running"
fi

# Check and open Terminal
if ! pgrep -x "gnome-terminal" > /dev/null; then
    echo "Opening Terminal..."
    gnome-terminal &
else
    echo "Terminal is already running"
fi

# Check and open Chrome
if ! pgrep -x "chrome" > /dev/null; then
    echo "Opening Chrome..."
    google-chrome &
else
    echo "Chrome is already running"
fi

echo "Done!"