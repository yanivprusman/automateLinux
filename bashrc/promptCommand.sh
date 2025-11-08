current_dir=$(pwd)
existing_dir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
if [ "$current_dir" != "$existing_dir" ]; then
    echo "last dir $existing_dir, appending $current_dir"
    # echo "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"B4
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
    insertDirAtIndex $current_dir $AUTOMATE_LINUX_DIR_HISTORY_POINTER
fi 
# echo $AUTOMATE_LINUX_DIR_HISTORY_POINTER
