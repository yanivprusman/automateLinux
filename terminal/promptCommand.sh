#pd and pdd commands
currentDir=$(pwd)
if [ ! -f "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
    insertDirAfterIndex $currentDir $AUTOMATE_LINUX_DIR_HISTORY_POINTER
fi
if [ ! -f "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" ]; then
    >"$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
fi
pointedToDir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
if [ -z "$pointedToDir" ]; then
   AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
fi
if [ "$currentDir" != "$pointedToDir" ]; then #in pd case this evaluates to false since pwd will return the same directory that's at the current history pointer
    insertDirAfterIndex $currentDir $AUTOMATE_LINUX_DIR_HISTORY_POINTER
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
    totalLines=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
    if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -gt "$totalLines" ]; then
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$totalLines
    fi
    setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
else
    touch "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
fi 

#ps1


