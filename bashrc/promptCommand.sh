#pd and pdd commands
current_dir=$(pwd)
existing_dir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
if [ "$current_dir" != "$existing_dir" ]; then
    insertDirAfterIndex $current_dir $AUTOMATE_LINUX_DIR_HISTORY_POINTER
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
    else
    touch "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
fi 
#ps1


