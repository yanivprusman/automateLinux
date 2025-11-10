. "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}dirHistory.sh"
generatePassword() { python3 ~/generatePassword.py; }

h() {
    history | grep "$@"
}

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

runSingleton() {
    local SCRIPT="$1"
    if [ -f "$SCRIPT" ] && ! pgrep -f "$SCRIPT" > /dev/null; then
        # bash "$SCRIPT" &
        "$SCRIPT" &
        # echo "Started $SCRIPT"
    else
        :
        # echo "$SCRIPT is already running or does not exist."
    fi
}

