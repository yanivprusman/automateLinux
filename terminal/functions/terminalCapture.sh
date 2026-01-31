# Terminal output capture and clipboard functions

# Initialize terminal capture - redirects stdout/stderr to tee
initTerminalCapture() {
    mkdir -p "$AUTOMATE_LINUX_TERMINAL_CAPTURE_DIR" 2>/dev/null

    # Truncate/initialize the session file
    : > "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"

    # Redirect stdout and stderr through tee to capture file
    exec > >(tee -a "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE") 2>&1
}
export -f initTerminalCapture

# Strip ANSI escape sequences and control characters from text
stripEscapeSequences() {
    sed -E \
        -e 's/\x1b\[[0-9;?]*[A-Za-z]//g' \
        -e 's/\x1b\][^\x07]*(\x07|\x1b\\)//g' \
        -e 's/\x1b\([A-Z]//g' \
        -e 's/\x08//g' \
        -e 's/\x0d//g' \
        -e 's/\x7f//g' \
        -e 's/[\x00-\x08\x0b\x0c\x0e-\x1a]//g'
}
export -f stripEscapeSequences

# terminalToClipboard - copy terminal output to clipboard
# Usage:
#   terminalToClipboard           - Copy last non-empty line before prompt
#   terminalToClipboard N         - Copy line N before prompt (1=last line)
#   terminalToClipboard N-M       - Copy lines N through M before prompt
#   terminalToClipboard fromPrompt - Copy all from previous prompt to current
terminalToClipboard() {
    local arg="$1"
    local arg2="$2"
    local capture_file="$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"
    local marker_pattern="^---PROMPT\[timestamp:[0-9]+\]---$"

    if [ ! -f "$capture_file" ]; then
        return 1
    fi

    local -a lines
    mapfile -t lines < "$capture_file"
    local total=${#lines[@]}

    # Find last two prompt markers
    local last_prompt=-1 prev_prompt=-1
    for ((i = total - 1; i >= 0; i--)); do
        if [[ "${lines[$i]}" =~ $marker_pattern ]]; then
            if [ $last_prompt -eq -1 ]; then
                last_prompt=$i
            else
                prev_prompt=$i
                break
            fi
        fi
    done

    [ $last_prompt -eq -1 ] && return 1

    # Build array of non-marker lines (indexes into original lines array)
    local -a content_indexes
    for ((i = 0; i < last_prompt; i++)); do
        [[ ! "${lines[$i]}" =~ $marker_pattern ]] && content_indexes+=($i)
    done

    local content_count=${#content_indexes[@]}
    [ $content_count -eq 0 ] && return 1

    local to_copy=""
    local start_idx end_idx

    case "$arg" in
        -h|--help)
            echo "Usage: terminalToClipboard [OPTION]"
            echo "Copy terminal output to clipboard (escape sequences stripped)"
            echo ""
            echo "  (none), 1    Last non-empty line"
            echo "  N            Line N from end (1=last, 2=second-to-last, ...)"
            echo "  N-M          Lines N through M from end"
            echo "  fromPrompt [N]     Output from Nth previous command (default: 1)"
            echo "  fromPrompt N-M     Output from commands N through M (e.g., 2-1)"
            echo "  -h, --help   Show this help"
            return 0
            ;;
        ""|1)
            # Default or 1: last non-empty content line
            for ((i = content_count - 1; i >= 0; i--)); do
                local idx=${content_indexes[$i]}
                if [ -n "${lines[$idx]}" ]; then
                    to_copy="${lines[$idx]}"
                    break
                fi
            done
            ;;
        fromPrompt)
            # Optional second argument: N or N-M (e.g., "ttc fromPrompt 2" or "ttc fromPrompt 2-1")
            local fp_arg="$arg2"

            # Find all prompt markers
            local -a prompt_indexes
            for ((i = 0; i < total; i++)); do
                [[ "${lines[$i]}" =~ $marker_pattern ]] && prompt_indexes+=($i)
            done
            local prompt_count=${#prompt_indexes[@]}
            [ $prompt_count -lt 2 ] && return 1

            # Determine which prompts to use (1 = most recent command, 2 = one before, etc.)
            local start_prompt=1 end_prompt=1
            if [[ "$fp_arg" =~ ^[0-9]+-[0-9]+$ ]]; then
                start_prompt="${fp_arg%-*}"
                end_prompt="${fp_arg#*-}"
            elif [[ "$fp_arg" =~ ^[0-9]+$ ]]; then
                start_prompt="$fp_arg"
                end_prompt="$fp_arg"
            fi

            # Ensure start >= end (start is further back)
            [ $start_prompt -lt $end_prompt ] && { local t=$start_prompt; start_prompt=$end_prompt; end_prompt=$t; }

            # Convert to indexes: prompt N means prompt_indexes[prompt_count - 1 - N]
            local start_idx=$((prompt_count - 1 - start_prompt))
            local end_idx=$((prompt_count - 1 - end_prompt))
            [ $start_idx -lt 0 ] && start_idx=0
            [ $end_idx -ge $prompt_count ] && end_idx=$((prompt_count - 1))

            # Copy content from start_prompt+1 to end_prompt (exclusive of end marker)
            local collect_from=$((${prompt_indexes[$start_idx]} + 1))
            local collect_to=${prompt_indexes[$((end_idx + 1))]}
            [ $((end_idx + 1)) -ge $prompt_count ] && collect_to=$total

            for ((i = collect_from; i < collect_to; i++)); do
                [[ ! "${lines[$i]}" =~ $marker_pattern ]] && to_copy+="${lines[$i]}"$'\n'
            done
            ;;
        *-*)
            # Range N-M (counting non-marker lines from end, 1=last)
            local n="${arg%-*}" m="${arg#*-}"
            start_idx=$((content_count - n))
            end_idx=$((content_count - m))
            [ $start_idx -gt $end_idx ] && { local t=$start_idx; start_idx=$end_idx; end_idx=$t; }
            [ $start_idx -lt 0 ] && start_idx=0
            [ $end_idx -ge $content_count ] && end_idx=$((content_count - 1))
            for ((i = start_idx; i <= end_idx; i++)); do
                to_copy+="${lines[${content_indexes[$i]}]}"$'\n'
            done
            ;;
        [0-9]*)
            # Line N from end (counting non-marker lines, 1=last)
            local idx=$((content_count - arg))
            [ $idx -ge 0 ] && [ $idx -lt $content_count ] && to_copy="${lines[${content_indexes[$idx]}]}"
            ;;
        *)
            return 1
            ;;
    esac

    to_copy=$(echo -n "$to_copy" | stripEscapeSequences)
    to_copy="${to_copy%$'\n'}"

    if [ -n "$to_copy" ]; then
        echo -n "$to_copy" | xclip -selection clipboard
    else
        return 1
    fi
}
export -f terminalToClipboard
alias ttc='terminalToClipboard'
alias ttcc='terminalToClipboard fromPrompt'
