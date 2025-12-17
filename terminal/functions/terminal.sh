# Terminal and shell utilities

resetPromptColor() {
    echo -en "\033[00m"
}
export -f resetPromptColor

# cd() {
#     if [[ "$1" =~ ^\.{2,}$ ]]; then
#         local levels=$(( ${#1} - 1 ))
#         local path=
#         for ((i=0; i<levels; i++)); do
#             path+="../"
#         done
#         builtin cd "${path%/}"
#     else
#         builtin cd "$@"
#     fi
# }
cd () {
    if [[ "$1" =~ ^\.{2,}$ ]]; then
        local levels=$(( ${#1} - 1 ))
        local path=
        for ((i=0; i<levels; i++)); do
            path+="../"
        done
        builtin cd "${path%/}"
    else
        if [[ -d "$1" ]]; then
            builtin cd "$1"/
        else
            builtin cd "$@"
        fi
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

getCurrentShell(){
    readlink /proc/$$/exe
    # or echo $0 ?
}
export -f getCurrentShell

changeTitle() {
    local prepend="" append="" replace="" reset=""
    while [ "$#" -gt 0 ]; do
        case "$1" in
            -prepend) prepend="$2"; shift 2;;
            -append)  append="$2"; shift 2;;
            -replace) replace="$2"; shift 2;;
            -reset) reset="1"; shift 1;;
            *) return 1;;
        esac
    done
    [ -n "$reset" ] && PS1="$AUTOMATE_LINUX_PS1" && return 0
    local current_title
    current_title=$(printf "%s" "$PS1" | sed -n 's/.*\\\[\\e]0;\(.*\)\\a.*/\1/p')
    [ -z "$current_title" ] && return 0
    local new_title="$current_title"
    [ -n "$prepend" ] && new_title="${prepend}${new_title}"
    [ -n "$append" ]  && new_title="${new_title}${append}"
    [ -n "$replace" ] && new_title="${replace}"
    PS1=$(printf "%s" "$PS1" |
    sed 's#\\\[\\e]0;.*\\a#\\[\\e]0;'"$(printf "%s" "$new_title" | sed 's/[\&]/\\&/g')"'\\a#')
}
export -f changeTitle

timeCd() {
    local start end
    start=$(date +%s.%N)
    cd .. || return
    end=$(date +%s.%N)
    echo "cd total time: $(echo "$end - $start" | bc) seconds"
    cd -
}
export -f timeCd
setPS1ToDirName() {
    PS1='$(basename "$PWD")\$ '
}
export -f setPS1ToDirName

whichReal(){
    realpath $(which "$1")
}
export -f whichReal