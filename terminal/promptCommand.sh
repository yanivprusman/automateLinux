# Directory history management
current_dir=$(pwd)

# Ensure history directory exists
mkdir -p "$(dirname "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")"

# Handle new or empty history file
if [ ! -s "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
    echo "$current_dir" > "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
    setDirHistoryPointer "$AUTOMATE_LINUX_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
else
    # Check if we're at a valid position in history
    existing_dir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
    
    # If we can't get the current directory from history, reset to end
    if [ -z "$existing_dir" ]; then
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
        [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -lt 1 ] && AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
        setDirHistoryPointer "$AUTOMATE_LINUX_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
        existing_dir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
    fi

    # Only add new directory if it's different from current
    if [ "$current_dir" != "$existing_dir" ]; then
        insertDirAfterIndex "$current_dir" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
        setDirHistoryPointer "$AUTOMATE_LINUX_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
    fi
fi
#ps1


