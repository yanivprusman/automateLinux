# Terminal and shell utilities

resetPromptColor() {
    echo -en "\033[00m"
}
export -f resetPromptColor

cd() {
    if [ "$1" = "..." ]; then
        builtin cd ../..
    else
        builtin cd "$@"
    fi
}
export -f cd

outputToSelf(){
    exec 1>/dev/pts/$(tty | sed 's:/dev/pts/::')
}
export -f outputToSelf

showTime(){
    date +%H:%M
}
export -f showTime

echoBlockSeparator() {
    echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
}
export -f echoBlockSeparator

