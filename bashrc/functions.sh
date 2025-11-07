cd() { builtin pushd "$@" > /dev/null; }
pd() { popd > /dev/null; }
resetPromptColor() {
    echo -en "\033[00m"
}
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

setEmoji() {
    export PROMPT_EMOJI=true
}

setDirHistoryPointerToLast() {
    if [[ -f "$AUTOMAT_LINUX_DIR_HISTORY_FILE" ]]; then
        line_count=$(wc -l < "$AUTOMAT_LINUX_DIR_HISTORY_FILE" | tr -d ' ')
        AUTOMAT_LINUX_DIR_HISTORY_POINTER=$line_count
    else
        AUTOMAT_LINUX_DIR_HISTORY_POINTER=1
    fi
    if [[ -z "$AUTOMAT_LINUX_DIR_HISTORY_POINTER" || "$AUTOMAT_LINUX_DIR_HISTORY_POINTER" -lt 1 ]]; then
        AUTOMAT_LINUX_DIR_HISTORY_POINTER=1
    fi
    # if [[ -f "$AUTOMAT_LINUX_DIR_HISTORY_FILE" && "$AUTOMAT_LINUX_DIR_HISTORY_POINTER" -gt "$line_count" ]]; then
    #     AUTOMAT_LINUX_DIR_HISTORY_POINTER=$line_count
    # fi
    # echo "$AUTOMAT_LINUX_DIR_HISTORY_FILE"
    # echo "$AUTOMAT_LINUX_DIR_HISTORY_POINTER"
    # echo "$line_count"
}

goToDirPointer(){
    if [ -f "$AUTOMAT_LINUX_DIR_HISTORY_FILE" ]; then
        lastDir=$(sed -n "${AUTOMAT_LINUX_DIR_HISTORY_POINTER}p" "$AUTOMAT_LINUX_DIR_HISTORY_FILE")
        if [ -d "$lastDir" ]; then
            cd "$lastDir" >/dev/null 2>&1
        fi
    fi
}