resetPromptColor() {
    echo -en "\033[00m"
}
export -f resetPromptColor
trap 'resetPromptColor' DEBUG

printEmojis(){
    for code in $(seq 0x1F300 0x1FAFF); do
        printf "\\U$(printf '%08X' $code)"
        # Print a newline every 20 emojis
        if (( (code - 0x1F300 + 1) % 20 == 0 )); then
            printf "\n"
        fi
    done
}
export -f printEmojis
# printEmojiCodes() {
#     for emoji in "$@"; do
#         code=$(python3 -c "print('U+{:X}'.format(ord('$emoji')))")
#         echo "$emoji $code"
#     done
# }

printEmojiCodes() {
    for emoji in "$@"; do
        code=$(python3 -c "print('{:X}'.format(ord('$emoji')))")
        echo "Emoji: $emoji"
        echo "Unicode: U+$code"
        echo "To print in Bash:"
        echo "  echo -e \"\\U$(printf '%08X' 0x$code)\""
        echo "  printf \"\\U$(printf '%08X' 0x$code)\\n\""
        echo
    done
}
export -f printEmojiCodes

setEmoji() {
    export PROMPT_EMOJI=true
}
export -f setEmoji

initializeDirHistoryFileTty() {
    AUTOMATE_LINUX_DIR_HISTORY_FILE_LAST_CHANGED=$(ls -1t "${AUTOMATE_LINUX_DIR_HISTORY_FILE_BASE}"* 2>/dev/null | head -n 1)
    if [[ -f "$AUTOMATE_LINUX_DIR_HISTORY_FILE_LAST_CHANGED" ]]; then
        if [ "$AUTOMATE_LINUX_DIR_HISTORY_FILE_LAST_CHANGED" != "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
            cp "$AUTOMATE_LINUX_DIR_HISTORY_FILE_LAST_CHANGED" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
        fi
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" | tr -d ' ')
        if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -lt 1 ]; then
            AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
        fi
    else 
        touch "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
        AUTOMATE_LINUX_DIR_HISTORY_POINTER=1
    fi 
}
export -f initializeDirHistoryFileTty

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
    local dir="$1"
    local index="$2"
    local sedICommand="$3"
    if [[ -z "$dir" || -z "$index" ]]; then
        echo "Usage: insertDirAtIndex <directory> <index>"
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
        goToDirPointer
    fi
}
export -f pdd

setDirHistoryPointer() {
    local ttyPath="$1"
    local pointerValue="$2"
    if grep -q "^$ttyPath " "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"; then
        awk -v tty="$ttyPath" -v p="$pointerValue" \
            '$1 == tty {$2="pointer:"p} {print}' "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" > "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE.tmp" && mv "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE.tmp" "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
    else
        echo "$ttyPath pointer:$pointerValue" >> "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
    fi
}

getDirHistoryPointer() {
    local ttyPath="$1"
    awk -v tty="$ttyPath" '$1 == tty {print $2}' "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
}
