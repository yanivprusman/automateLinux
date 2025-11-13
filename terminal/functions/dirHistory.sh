resetDirHistory() {
    rm "${AUTOMATE_LINUX_DATA_DIR}dirHistory/"* 2>/dev/null
}

initializeDirHistory() { 
    local lastChanged
    lastChanged=$(ls -1t "${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}"* 2>/dev/null | grep -Fxv "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" | head -n 1)
    if [[ -f "$lastChanged" ]]; then
        if [ "$lastChanged" != "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
            cp "$lastChanged" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
            # lastChangedTty=${${lastChanged#${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}}%.*}
            lastChangedTty=$(basename "${lastChanged#${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}}" .*)
            local savedPointer=$(getDirHistoryPointer "$lastChangedTty")
            if [[ $savedPointer =~ pointer:([0-9]+) ]]; then
                AUTOMATE_LINUX_DIR_HISTORY_POINTER="${BASH_REMATCH[1]}"
            else
                AUTOMATE_LINUX_DIR_HISTORY_POINTER=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" | tr -d ' ')
            fi
            if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -lt 1 ]; then
                AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
            fi
            local totalLines=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" | tr -d ' ')
            if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -gt "$totalLines" ]; then
                AUTOMATE_LINUX_DIR_HISTORY_POINTER=$totalLines
            fi
        else
            # lastChanged="$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=
        fi
    else 
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=1

        # > "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
        # > "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
        # lastChanged="$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    fi 
    setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
}
export -f initializeDirHistory

goToDirPointer(){
    # echo "Going to dir pointer $AUTOMATE_LINUX_DIR_HISTORY_POINTER"
    if [ -f "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
        lastDir=$(sed -n "${AUTOMATE_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY")
        if [ -d "$lastDir" ]; then
            cd "$lastDir" >/dev/null 2>&1
        fi
    fi
}
export -f goToDirPointer

insertDir(){
    local dir="$1" index="$2" sedICommand="$3"
    if [[ -z "$dir" || -z "$index" ]]; then
        return 1
    fi
    if [ -s "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
        sed -i "${index}$sedICommand$dir" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    else
        echo "$dir" > "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    fi
}
export -f insertDir

insertDirAtIndex(){
    insertDir "$1" "$2" "i"
}
export -f insertDirAtIndex

insertDirAfterIndex(){
    insertDir "$1" "$2" "a"
}
export -f insertDirAfterIndex

pd() {
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER - 1))
    if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -lt 1 ]; then
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
    fi  
    setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
    goToDirPointer     
}
export -f pd

pdd() {
    if [ -f "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
        totalLines=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" | tr -d ' ')
        if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -ge "$totalLines" ]; then
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=$totalLines
        fi
        setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
        goToDirPointer
    fi
}
export -f pdd

setDirHistoryPointer() {
    local escapedTty="$1"
    local pointer="$2"
    if [ ! -f "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" ]; then
        > "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
    fi
    if grep -q "^$escapedTty " "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"; then
        sed -i "s|^$escapedTty .*|$escapedTty $pointer|" "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
    else
        echo "$escapedTty $pointer" >> "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
    fi
}
export -f setDirHistoryPointer

getDirHistoryPointer() {
    local escapedTty="$1"
    awk -v escapedTty="$escapedTty" '$1 == escapedTty {print $2}' "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
}
export -f getDirHistoryPointer

