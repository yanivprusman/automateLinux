. "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}dirHistory.sh"
generatePassword() { python3 ~/generatePassword.py; }

h() {
    history | grep "$@"
}

resetPromptColor() {
    echo -en "\033[00m"
}
export -f resetPromptColor

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
        "$SCRIPT" &
    else
        :
    fi
}

cd() {
    if [ "$1" = "..." ]; then
        builtin cd ../..
    else
        builtin cd "$@"
    fi
}
export -f cd

touchDirectories() {
    local dir
    local automateLinuxVariable
    compgen -v | grep '^AUTOMATE_LINUX_' | grep '_DIR$' | while read -r automateLinuxVariable; do
        dir="${!automateLinuxVariable}"
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
        fi
    done
}
export -f touchDirectories

catDir() {
    local dir="$1"
    if [ -z "$dir" ]; then dir="."; fi
    if [ -d "$dir" ]; then
        for file in "$dir"*; do
            if [ -f "$file" ]; then
                echo -e "${green}----- Contents of $file:${NC}"
                cat "$file"
            fi
        done
    else
        echo "$dir is not a directory."
    fi
}
export -f catDir

heredoc() {
    local file="$1"
    if [ -f "$file" ]; then
        local outputFile=$(basename "${1%.*}").hereDoc
        # echo "Creating here document file: $outputFile"
        echo "bash <<'EOF'" > "$outputFile"
        cat "$file" >> "$outputFile"
        echo "EOF" >> "$outputFile"
    fi
}
export -f heredoc

showArrays() {
    for array in $(compgen -A arrayvar); do
        echo -e "${green}$array${nc}"
        eval echo "\${${array}[@]}"
        echo "----"
    done
}
export -f showArrays

showVars() {
    for var in $(compgen -v); do
        echo -e "${green}$var${NC} = ${!var}"
    done
        # ${!var}
        # eval echo "\${${var}}"
        # echo "----"
}
export -f showVars

printArray() {
    local arr_name=$1
    local keys
    eval "keys=(\"\${!${arr_name}[@]}\")"
    for key in "${keys[@]}"; do
        eval "echo \"$key='\${${arr_name}[${key}]}'\""
    done
}
export -f printArray


deleteFunctions() {
    for f in $(declare -F | awk '{print $3}'); do
        unset -f "$f"
    done
}
export -f deleteFunctions

printDir(){
    local dirs=()
    local files=()
    local dir f
    local -A excluded_files=()
    local exclude_file=".printDir.sh"
    local mode="dirs"
    while [ $# -gt 0 ]; do
        if [ "$1" = "-d" ]; then
            mode="dirs"
            shift
        elif [ "$1" = "-f" ]; then
            mode="files"
            shift
        elif [ "$mode" = "dirs" ]; then
            dirs+=("$1")
            shift
        else
            files+=("$1")
            shift
        fi
    done
    if [ ${#dirs[@]} -eq 0 ] && [ ${#files[@]} -eq 0 ]; then
        dirs=(".")
    fi
    for dir in "${dirs[@]}"; do
        if [ -d "$dir" ] && [ -f "$dir/$exclude_file" ]; then
            while IFS= read -r line || [ -n "$line" ]; do
                line="${line%%#*}"
                line="${line%"${line##*[![:space:]]}"}"
                line="${line#"${line%%[![:space:]]*}"}"
                [ -z "$line" ] && continue
                excluded_files["$line"]=1
            done < "$dir/$exclude_file"
        fi
    done
    if [ ${#dirs[@]} -gt 0 ]; then
        for dir in "${dirs[@]}"; do
            if [ -d "$dir" ]; then
                for f in "${dir%/}/"*; do
                    if [ -f "$f" ]; then
                        local basename_f
                        basename_f=$(basename "$f")
                        [ -z "${excluded_files[$basename_f]}" ] || continue
                        echo -e "${GREEN}$basename_f:${NC}"
                        cat "$f"
                        echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
                    fi
                done
            fi
        done
    fi
    if [ ${#files[@]} -gt 0 ]; then
        for f in "${files[@]}"; do
            if [ -f "$f" ]; then
                local basename_f
                basename_f=$(basename "$f")
                [ -z "${excluded_files[$basename_f]}" ] || continue
                echo -e "${GREEN}$basename_f:${NC}"
                cat "$f"
                echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
            fi
        done
    fi
}
export -f printDir

b(){
    ./build.sh "$@"
}
export -f b

m(){
    build/main "$@"
}
export -f m

outputToSelf(){
    exec 1>/dev/pts/$(tty | sed 's:/dev/pts/::')
}
export -f outputToSelf

copyToClipboard(){
    (xclip -selection clipboard)
}
export -f copyToClipboard

isFile(){
    local file="$1"
    if [ -f "$file" ]; then
        echo true
    else
        echo false
    fi
}
export -f isFile

d(){
    echo "in d function"
    daemon "$@"
}
export -f d

daemon() {
    if [ ! -S "$AUTOMATE_LINUX_SOCKET_PATH" ]; then
        return 0
    fi
    local COMMAND="$1" json key value response
    shift
    json="{\"command\":\"$COMMAND\""
    for arg in "$@"; do
        key="${arg%%=*}"
        value="${arg#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"
    echo "$json" >&"${DAEMON_CO[1]}"
    read -r response <&"${DAEMON_CO[0]}"
    echo "$response"
    # echo "$json" >&3
    # read -r response <&3
    # echo "$response"
    # echo "$json" > "$PIPE"
    # ./sendJson "$AUTOMATE_LINUX_SOCKET_PATH" "$json"
    # echo "$json" > /proc/$NC_PID/fd/0
    # echo "$json" >& /proc/$NC_PID/fd/0
    # echo "$json" | nc -U "$AUTOMATE_LINUX_SOCKET_PATH"
    # echo "$json" | jq . | nc -U "$AUTOMATE_LINUX_SOCKET_PATH"
}
export -f daemon

# alias time=showTime
showTime(){
    date +%H:%M
}
export -f showTime

status(){
    systemctl status "$@"
}
export -f status

start(){
    systemctl start "$@"
}
export -f start

reload(){
    systemctl daemon-reload
}
export -f reload

toSymbolic() {
    local oct="$1"
    python3 -c "import stat; print(stat.filemode(int('$oct', 8)))"
}
export -f toSymbolic

echoBlockSeparator() {
    echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
}
export -f echoBlockSeparator    

killAllJobs() {
    # for job in $(jobs -p; jobs | awk '/Stopped/ {print $1}' | tr -d '[]+%'); do
    #     kill -CONT %$job 2>/dev/null   # resume if stopped
    #     kill -9 %$job 2>/dev/null      # then force kill
    # done
    for job in $(jobs -p); do
        kill -9 $job 2>/dev/null
    done

}
export -f killAllJobs

# initCoproc() {
#     jobs -p | xargs kill 2>/dev/null
#     coproc DAEMON_CO { nc -U "$AUTOMATE_LINUX_SOCKET_PATH"; }
# }
initCoproc() {
    # kill any previous background coproc process
    if [[ -n "${DAEMON_CO_PID-}" ]] && kill -0 "$DAEMON_CO_PID" 2>/dev/null; then
        kill "$DAEMON_CO_PID" 2>/dev/null
        wait "$DAEMON_CO_PID" 2>/dev/null
    fi
    # remove old coproc variables
    unset DAEMON_CO DAEMON_CO_PID
    # start new coproc
    coproc DAEMON_CO { nc -U "$AUTOMATE_LINUX_SOCKET_PATH"; }
    DAEMON_CO_PID=$!
}
export -f initCoproc
#  do not delete empty rows above this line