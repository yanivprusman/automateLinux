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

redirectSession() {
    local nameoffile="${AUTOMATE_LINUX_DATA_DIR}session${AUTOMATE_LINUX_TTY_NUMBER}.txt"
    : > "$nameoffile"
    exec > >(tee -a "$nameoffile") 2>&1
}
export -f redirectSession

restoreOutput() {
    exec > /dev/tty 2>&1
}
export -f restoreOutput

terminalToClipboard() {
    local arg1=$1
    local arg2=$2
    local nameoffile="${AUTOMATE_LINUX_DATA_DIR}session${AUTOMATE_LINUX_TTY_NUMBER}.txt"

    if [ ! -f "$nameoffile" ]; then
        restoreOutput
        echo "Session log file not found: $nameoffile"
        redirectSession
        return 1
    fi

    local start_idx=1
    local end_idx=1

    # Argument parsing
    if [[ -z "$arg1" ]]; then
        start_idx=1
        end_idx=1
    elif [[ "$arg1" =~ ^([0-9]+)-([0-9]+)$ ]]; then
        start_idx=${BASH_REMATCH[1]}
        end_idx=${BASH_REMATCH[2]}
    elif [[ -n "$arg1" && -n "$arg2" ]]; then
        start_idx=$arg1
        end_idx=$arg2
    else
        start_idx=$arg1
        end_idx=$arg1
    fi

    if [ "$start_idx" -lt "$end_idx" ]; then
        local tmp=$start_idx
        start_idx=$end_idx
        end_idx=$tmp
    fi

    local lines_collected=()
    local current_idx=0
    local found_command_line=0
    
    # Process the file in reverse using tac (read last 2000 lines to verify)
    while IFS= read -r raw_line; do
        # Clean the line
        local clean_line=$(echo "$raw_line" | perl -pe 's/\x1b\[[0-9;?]*[@-~]//g; s/\x1b\].*?(\x07|\x1b\\)//g; s/\r//g; 1 while s/[^\x08]\x08//g; s/^\x08+//g')
        
        # If empty (noise), skip
        if [[ -z "${clean_line// }" ]]; then
            continue
        fi

        # Skip the command line itself. 
        # Instead of just blindly skipping the first non-empty line, check if it looks like the command.
        # But user might have typed " terminalToClipboard 5" (spaces).
        # Or aliases output.
        # The safest heuristic for the user's "blind" usage is still "Skip the last line entered".
        # But if the user pressed Enter on an empty prompt before, that might be captured?
        # Clean line removes prompts? NO. The prompt is in the file.
        # "user@host:~$ terminalToClipboard"
        # We should skip the line containing "terminalToClipboard" if it's the first one we see.
        
        if [ "$found_command_line" -eq 0 ]; then
            if [[ "$clean_line" == *"terminalToClipboard"* ]]; then
                found_command_line=1
                continue
            fi
            # If we see a prompt ending in $, it might be the command line where user didn't type anything yet? 
            # Or the command just finished.
            # Let's stick to the "Skip 1" rule but maybe log what we skipped for debug if strictly needed?
            # User wants silence.
            # Let's trust the "Skip 1" rule BUT strictly skip the line with the command.
            # Actually, the user's log showed the command line looks like:
            # "^[[?2004h^[]0;...$ terminalToClipboard ..."
            # So searching for "terminalToClipboard" in the last few lines is safer.
            found_command_line=1
            continue
        fi
        
        current_idx=$((current_idx + 1))
        
        if [ "$current_idx" -ge "$end_idx" ] && [ "$current_idx" -le "$start_idx" ]; then
            lines_collected+=("$clean_line")
        fi
        
        if [ "$current_idx" -gt "$start_idx" ]; then
            break
        fi

    done < <(tail -n 2000 "$nameoffile" | tac)
    
    if [ ${#lines_collected[@]} -gt 0 ]; then
        local output=""
        for (( i=${#lines_collected[@]}-1; i>=0; i-- )); do
             output+="${lines_collected[$i]}"
             if [ $i -gt 0 ]; then output+=$'\n'; fi
        done
        echo -n "$output" | xclip -selection clipboard
    else
        restoreOutput
        echo "No valid lines found in range."
        redirectSession
        return 1
    fi
}
export -f terminalToClipboard
