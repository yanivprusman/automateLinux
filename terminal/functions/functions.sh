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
        # bash "$SCRIPT" &
        "$SCRIPT" &
        # echo "Started $SCRIPT"
    else
        :
        # echo "$SCRIPT is already running or does not exist."
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
    local dir f
    if [ $# -eq 0 ]; then
        dirs=(".")
    else
        while [ $# -gt 0 ]; do
            dirs+=("$1")
            shift
        done
    fi
    for dir in "${dirs[@]}"; do
        if [ -d "$dir" ]; then
            for f in "${dir%/}/"*; do
                if [ -f "$f" ]; then
                    echo -e "${GREEN}$(basename "$f"):${NC}"
                    cat "$f"
                    echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
                fi
            done
        fi
    done
}

b(){
    ./build.sh "$@"
}

m(){
    build/main "$@"
}

#  do not delete empty rows above this line