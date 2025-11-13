# when ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE 
# promptCommand
# pd
# pdd

truncatePointersFile() { 
    > "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" 
}

resetWithDefaultDir() {
    echo "$AUTOMATE_LINUX_DIR_HISTORY_DEFAULT_DIR">"$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"
}


getEscapedTty() {
    echo $(basename "${1#${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}}" .sh)
}
export -f getEscapedTty

testIfProper() {
    local ttyFile totalLines pointer
    ttyFile="$1"
    tty=$(getEscapedTty "$ttyFile")
    if [ -f "$ttyFile" ]; then
        totalLines=$(wc -l < "$ttyFile" | tr -d ' ')
        pointer=$(getDirHistoryPointer "$tty")
        if ! [[ $pointer =~ ^[0-9]+$ ]]; then
            return 1
        else
            if [ "$pointer" -lt 1 ] || [ "$pointer" -gt "$totalLines" ]; then
                return 1
            else
                return 0
            fi
        fi
    else
        return 1
    fi
}

resetDirHistoryToBeginningState() {
    rm "${AUTOMATE_LINUX_DATA_DIR}dirHistory/"* 2>/dev/null
    resetWithDefaultDir
    AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
    setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
}

initializeDirHistory() { 
    # set -x
    local lastChanged tty pointer totalLines
    if [[ ! -f "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" ]]; then
        resetDirHistoryToBeginningState
        return
    fi
    lastChanged=$(ls -1t "${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}"* 2>/dev/null | grep -Fxv "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" | head -n 1)
    if [[ ! -f "$lastChanged" ]]; then
        resetDirHistoryToBeginningState
        return
    else
        if [[ "$lastChanged" == "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE" ]]; then
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=$(getDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY")
        else
            tty=$(getEscapedTty "$lastChanged")
            cp "$lastChanged" "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=$(getDirHistoryPointer "$tty")
            setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
        fi
    fi
    if ! testIfProper "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"; then
        resetDirHistoryToBeginningState
    fi
    set +x
}
export -f initializeDirHistory

cdToPointer(){
    local lastDir
    # echo "Going to dir pointer $AUTOMATE_LINUX_DIR_HISTORY_POINTER"
    if [ -f "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE" ] && [ -n "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" ]; then
        lastDir=$(awk "NR==${AUTOMATE_LINUX_DIR_HISTORY_POINTER}" "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE")
        if [ -d "$lastDir" ]; then
            cd "$lastDir" >/dev/null 2>&1
        fi
    fi
}
export -f cdToPointer

insertDir(){
    local dir="$1" index="$2" sedICommand="$3"
    if [[ -z "$dir" || -z "$index" ]]; then
        return 1
    fi
    if [ -s "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE" ]; then
        sed -i "${index}$sedICommand$dir" "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"
    else
        echo "$dir" > "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"
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
    cdToPointer     
}
export -f pd

pdd() {
    if [ -f "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE" ]; then
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
        totalLines=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE" | tr -d ' ')
        if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -ge "$totalLines" ]; then
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=$totalLines
        fi
        setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
        cdToPointer
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

