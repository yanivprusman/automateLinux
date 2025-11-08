# add export -f  to all functions

# pd() { popd > /dev/null; }
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
        if [ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -eq 0 ]; then
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

insertDirAtIndex(){
    # echo inserting directory "$1" at index "$2"
    local dir="$1"
    local index="$2"
    if [[ -z "$dir" || -z "$index" ]]; then
        echo "Usage: insertDirAtIndex <directory> <index>"
        return 1
    fi
    if [ -s "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY" ]; then
        echo "true"
        sed -i "${index}i$dir" "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    else
        echo "false"
        echo "$dir" > "$AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    fi

    # echo "Inserting $dir at line $index in $AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY"
    # echo "Inserted $dir at line $index in $AUTOMATE_LINUX_DIR_HISTORY_FILE"
}
export -f insertDirAtIndex

