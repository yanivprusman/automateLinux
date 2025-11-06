alias mk='mkdir'
alias boot='history -a && sudo reboot'
alias pkill='sudo pkill -9'
alias pkillev='sudo pkill -9 evsieve'
alias evsievep='sudo evsieve --input /dev/input/event* --print format=direct'
alias 1restart='restartCorsairKeyBoardLogiMouseService.sh reset'
alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'
alias trr='tr ":" "\n"'
alias sourceb='source ~/.bashrc'
alias lstr='ls --color=always | tr " " "\n"'
alias cls='clear'
alias cd..='cd ..'
alias cdc='cd /home/yaniv/coding/'
PS1_SCRIPT="$(dirname "${BASH_SOURCE[0]}")/ps1.sh"
PROMPT_COMMAND='PS1=$(bash "'"$PS1_SCRIPT"'")'
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