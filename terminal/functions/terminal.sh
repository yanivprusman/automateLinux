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

# terminalToClipboard() {
#     # 1. Clear clipboard to ensure we don't use old data if capture fails
#     if command -v wl-copy &> /dev/null; then
#         echo -n "" | wl-copy
#     elif command -v xclip &> /dev/null; then
#         echo -n "" | xclip -selection clipboard
#     fi

#     # 2. Simulate Select All (Ctrl+Shift+A) + Copy (Ctrl+Shift+C)
#     # KEY_LEFTCTRL=29, KEY_LEFTSHIFT=42, KEY_A=30, KEY_C=46
#     daemon send simulateInput --type 1 --code 29 --value 1 > /dev/null # Ctrl Down
#     daemon send simulateInput --type 1 --code 42 --value 1 > /dev/null # Shift Down
#     daemon send simulateInput --type 1 --code 30 --value 1 > /dev/null # A Down
#     daemon send simulateInput --type 1 --code 30 --value 0 > /dev/null # A Up
#     daemon send simulateInput --type 1 --code 46 --value 1 > /dev/null # C Down
#     daemon send simulateInput --type 1 --code 46 --value 0 > /dev/null # C Up
#     daemon send simulateInput --type 1 --code 42 --value 0 > /dev/null # Shift Up
#     daemon send simulateInput --type 1 --code 29 --value 0 > /dev/null # Ctrl Up

#     # 3. Wait for clipboard and read buffer
#     sleep 0.2
#     local buffer
#     if [[ "$XDG_SESSION_TYPE" == "wayland" ]]; then
#         buffer=$(wl-paste 2>/dev/null)
#     else
#         buffer=$(xclip -selection clipboard -o 2>/dev/null)
#     fi

#     if [[ -z "$buffer" ]]; then
#         return 0
#     fi

#     # 4. Clean and parse buffer
#     local clean_output
#     clean_output=$(echo "$buffer" | \
#         sed -r "s/\x1B\[[0-9;?]*[A-Za-z]//g" | \
#         sed -r "s/\x1B\].*(\x07|\x1B\\\\)//g" | \
#         sed -r "s/\x1B\(B//g" | \
#         col -b)

#     local lines
#     mapfile -t lines <<< "$clean_output"
#     local total=${#lines[@]}
    
#     # Find the current command line (the one running terminalToClipboard)
#     local current_idx=$((total - 1))
#     while [[ $current_idx -ge 0 && -z "${lines[$current_idx]}" ]]; do
#         ((current_idx--))
#     done

#     local start_idx=-1
#     local end_idx=-1

#     if [[ -z "$1" ]]; then
#         # Copy 1 line above
#         start_idx=$((current_idx - 1))
#         end_idx=$((current_idx - 1))
#     elif [[ "$1" == "fromPrompt" ]]; then
#         # Search backwards for the previous prompt
#         local i=$((current_idx - 1))
#         while [[ $i -ge 0 ]]; do
#             if [[ "${lines[$i]}" =~ [\$\#]\  ]]; then
#                 start_idx=$((i + 1))
#                 end_idx=$((current_idx - 1))
#                 break
#             fi
#             ((i--))
#         done
#         if [[ $start_idx -eq -1 ]]; then
#             start_idx=0
#             end_idx=$((current_idx - 1))
#         fi
#     elif [[ "$1" =~ ^([0-9]+)-([0-9]+)$ ]]; then
#         local n="${BASH_REMATCH[1]}"
#         local m="${BASH_REMATCH[2]}"
#         start_idx=$((current_idx - n))
#         end_idx=$((current_idx - m))
#     elif [[ "$1" =~ ^[0-9]+$ ]]; then
#         start_idx=$((current_idx - $1))
#         end_idx=$start_idx
#     fi

#     if [[ $start_idx -ge 0 && $end_idx -ge $start_idx ]]; then
#         local to_copy=""
#         for ((i=start_idx; i<=end_idx; i++)); do
#             local line="${lines[$i]}"
#             if [[ "$line" =~ ^[^\$\#]*[\$\#]\ (.*)$ ]]; then
#                 line="${BASH_REMATCH[1]}"
#             fi
#             to_copy+="$line"$'\n'
#         done
#         echo -n "$to_copy" | (command -v wl-copy &>/dev/null && wl-copy || xclip -selection clipboard)
#     fi
# }
# export -f terminalToClipboard

