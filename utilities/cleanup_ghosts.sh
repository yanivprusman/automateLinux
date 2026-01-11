#!/bin/bash
# cleanup_ghosts.sh
# Forcefully cleans up stuck overlay mounts and ghost user directories.

echo "Starting cleanup..."

# 1. Force Unmount everything in /home/*/
echo "Unmounting stuck overlays..."
sudo umount -l /home/*/.config/google-chrome 2>/dev/null
sudo umount -l /home/*/.config/Code 2>/dev/null
sudo umount -l /home/*/.vscode 2>/dev/null

# 2. Identify potential ghost directories (numeric/random names)
# We avoid 'yaniv' and 'lost+found'
GHOSTS=$(ls -d /home/*/ | grep -vE "/home/yaniv/|/home/lost\+found/")

if [ -z "$GHOSTS" ]; then
    echo "No potential ghost directories found."
else
    echo "Found potential ghosts:"
    echo "$GHOSTS"
    
    # 3. Aggressive Removal
    for dir in $GHOSTS; do
        user=$(basename "$dir")
        echo "Processing $user..."
        
        # Kill any lingering processes
        sudo pkill -9 -u "$user" 2>/dev/null
        
        # Delete user if exists
        sudo deluser --remove-home "$user" 2>/dev/null
        
        # Force remove directory if still there
        if [ -d "$dir" ]; then
            echo "Force removing $dir..."
            sudo rm -rf "$dir"
        fi
    done
fi

echo "Cleanup complete. Listing /home/:"
ls -ld /home/*/
