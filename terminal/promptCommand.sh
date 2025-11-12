#pd and pdd commands
current_dir=$(pwd)
existing_dir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
if [ "$current_dir" != "$existing_dir" ]; then #in pd case this evaluates to false since pwd will return the same directory that's at the current history pointer
    insertDirAfterIndex $current_dir $AUTOMATE_LINUX_DIR_HISTORY_POINTER
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
    setDirHistoryPointer "$AUTOMATE_LINUX_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
else
    touch "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
fi 

#ps1


