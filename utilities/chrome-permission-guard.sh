#!/bin/bash
# chrome-permission-guard.sh
# Watches the Chrome Preferences file and instantly fixes permissions when Chrome breaks them.

TARGET="/home/yaniv/.config/google-chrome/Default/Preferences"
USER="yaniv"
GROUP="coding"

echo "Guard active: Watching $TARGET for permission resets..."

# Loop forever waiting for events
while inotifywait -q -e close_write -e attrib "$TARGET"; do
    # When Chrome writes or changes attributes (chmod), we fight back instantly.
    # We use sudo -n (non-interactive) relying on our codingUsers sudoers rules.
    
    CURRENT_MASK=$(getfacl "$TARGET" 2>/dev/null | grep "mask::" | awk -F:: '{print $2}')
    
    if [ "$CURRENT_MASK" != "rwx" ]; then
        echo "$(date): Detected permission break. Forcing restore..."
        sudo chmod g+rw "$TARGET"
        sudo setfacl -m m:rwx "$TARGET"
    fi
done
