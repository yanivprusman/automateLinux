# typeMode - Multi-line text editor for quick clipboard typing
#
# A proper multi-line text editor with real cursor movement and text insertion.
# Type text, navigate with arrow keys, copy to clipboard with Ctrl+X.
#
# Key bindings:
#   Ctrl+X        - Copy all to clipboard, add to history, clear
#   Ctrl+U        - Clear all (no copy)
#   Ctrl+D        - Exit
#   Ctrl+Up/Down  - Navigate session history
#   Arrow keys    - Move cursor
#   Home/End      - Jump to line start/end
#   Ctrl+Left/Right - Word navigation
#   Backspace     - Delete char before cursor
#   Delete        - Delete char at cursor
#   Enter         - New line

typeMode() {
    # === State ===
    local -a lines=("")          # Array of lines (text split by newlines)
    local cursor_row=0           # Index into lines array
    local cursor_col=0           # Position within current line
    local -a history=()          # Session history (complete entries)
    local hist_idx=0             # History navigation position
    local old_stty

    # === Helper Functions ===

    # Get all text as a single string with newlines
    _tm_get_text() {
        local IFS=$'\n'
        printf '%s' "${lines[*]}"
    }

    # Set text from string, splitting into lines array
    _tm_set_text() {
        local text="$1"
        lines=()
        if [[ -z "$text" ]]; then
            lines=("")
            cursor_row=0
            cursor_col=0
            return
        fi
        # Split on newlines, preserving empty lines
        local IFS=$'\n'
        read -ra lines <<< "$text"
        # Handle trailing newline
        if [[ "$text" == *$'\n' ]]; then
            lines+=("")
        fi
        # Position cursor at end
        cursor_row=$(( ${#lines[@]} - 1 ))
        cursor_col=${#lines[$cursor_row]}
    }

    # Render all lines and position cursor
    _tm_render() {
        # Clear screen and move to home
        printf "\033[2J\033[H" >/dev/tty
        # Print all lines
        local i
        for ((i = 0; i < ${#lines[@]}; i++)); do
            printf '%s' "${lines[$i]}" >/dev/tty
            if ((i < ${#lines[@]} - 1)); then
                printf '\r\n' >/dev/tty
            fi
        done
        # Position cursor: move to home, then to correct row/col
        # Row 1 is line 0, etc.
        printf "\033[H" >/dev/tty
        if ((cursor_row > 0)); then
            printf "\033[%dB" "$cursor_row" >/dev/tty
        fi
        if ((cursor_col > 0)); then
            printf "\033[%dC" "$cursor_col" >/dev/tty
        fi
    }

    # Read one UTF-8 character from tty
    _tm_read_char() {
        local char rest
        # Read in C locale to get exactly one byte
        LC_ALL=C IFS= read -rsn1 char </dev/tty

        # Empty read (includes NUL byte which becomes empty string)
        if [[ -z "$char" ]]; then
            printf '%s' "$char"
            return
        fi

        # Get the numeric value of the byte
        local byte
        LC_ALL=C printf -v byte '%d' "'$char" 2>/dev/null || byte=0

        # Determine UTF-8 sequence length from lead byte
        if ((byte >= 240 && byte <= 247)); then
            # 4-byte sequence (11110xxx)
            LC_ALL=C IFS= read -rsn3 rest </dev/tty
            char+="$rest"
        elif ((byte >= 224 && byte <= 239)); then
            # 3-byte sequence (1110xxxx)
            LC_ALL=C IFS= read -rsn2 rest </dev/tty
            char+="$rest"
        elif ((byte >= 192 && byte <= 223)); then
            # 2-byte sequence (110xxxxx)
            LC_ALL=C IFS= read -rsn1 rest </dev/tty
            char+="$rest"
        fi
        # Single byte (0-127) or continuation byte (128-191) - just return char as-is

        printf '%s' "$char"
    }

    # Insert character at cursor position
    _tm_insert_char() {
        local char="$1"
        local line="${lines[$cursor_row]}"
        local before="${line:0:$cursor_col}"
        local after="${line:$cursor_col}"
        lines[$cursor_row]="$before$char$after"
        ((cursor_col++))
    }

    # Delete character before cursor (backspace)
    _tm_backspace() {
        if ((cursor_col > 0)); then
            # Delete within current line
            local line="${lines[$cursor_row]}"
            local before="${line:0:$((cursor_col - 1))}"
            local after="${line:$cursor_col}"
            lines[$cursor_row]="$before$after"
            ((cursor_col--))
        elif ((cursor_row > 0)); then
            # At start of line, join with previous
            local current_line="${lines[$cursor_row]}"
            local prev_line="${lines[$((cursor_row - 1))]}"
            cursor_col=${#prev_line}
            lines[$((cursor_row - 1))]="$prev_line$current_line"
            # Remove current line
            local -a new_lines=()
            local i
            for ((i = 0; i < ${#lines[@]}; i++)); do
                if ((i != cursor_row)); then
                    new_lines+=("${lines[$i]}")
                fi
            done
            lines=("${new_lines[@]}")
            ((cursor_row--))
        fi
    }

    # Delete character at cursor (Delete key)
    _tm_delete() {
        local line="${lines[$cursor_row]}"
        if ((cursor_col < ${#line})); then
            # Delete character at cursor
            local before="${line:0:$cursor_col}"
            local after="${line:$((cursor_col + 1))}"
            lines[$cursor_row]="$before$after"
        elif ((cursor_row < ${#lines[@]} - 1)); then
            # At end of line, join with next
            local next_line="${lines[$((cursor_row + 1))]}"
            lines[$cursor_row]="$line$next_line"
            # Remove next line
            local -a new_lines=()
            local i
            for ((i = 0; i < ${#lines[@]}; i++)); do
                if ((i != cursor_row + 1)); then
                    new_lines+=("${lines[$i]}")
                fi
            done
            lines=("${new_lines[@]}")
        fi
    }

    # Insert newline at cursor
    _tm_newline() {
        local line="${lines[$cursor_row]}"
        local before="${line:0:$cursor_col}"
        local after="${line:$cursor_col}"
        lines[$cursor_row]="$before"
        # Insert new line after current
        local -a new_lines=()
        local i
        for ((i = 0; i <= cursor_row; i++)); do
            new_lines+=("${lines[$i]}")
        done
        new_lines+=("$after")
        for ((i = cursor_row + 1; i < ${#lines[@]}; i++)); do
            new_lines+=("${lines[$i]}")
        done
        lines=("${new_lines[@]}")
        ((cursor_row++))
        cursor_col=0
    }

    # Move cursor left
    _tm_move_left() {
        if ((cursor_col > 0)); then
            ((cursor_col--))
        elif ((cursor_row > 0)); then
            ((cursor_row--))
            cursor_col=${#lines[$cursor_row]}
        fi
    }

    # Move cursor right
    _tm_move_right() {
        local line_len=${#lines[$cursor_row]}
        if ((cursor_col < line_len)); then
            ((cursor_col++))
        elif ((cursor_row < ${#lines[@]} - 1)); then
            ((cursor_row++))
            cursor_col=0
        fi
    }

    # Move cursor up
    _tm_move_up() {
        if ((cursor_row > 0)); then
            ((cursor_row--))
            local line_len=${#lines[$cursor_row]}
            if ((cursor_col > line_len)); then
                cursor_col=$line_len
            fi
        fi
    }

    # Move cursor down
    _tm_move_down() {
        if ((cursor_row < ${#lines[@]} - 1)); then
            ((cursor_row++))
            local line_len=${#lines[$cursor_row]}
            if ((cursor_col > line_len)); then
                cursor_col=$line_len
            fi
        fi
    }

    # Move to start of line
    _tm_home() {
        cursor_col=0
    }

    # Move to end of line
    _tm_end() {
        cursor_col=${#lines[$cursor_row]}
    }

    # Move word left (Ctrl+Left)
    _tm_word_left() {
        local line="${lines[$cursor_row]}"
        if ((cursor_col == 0)); then
            # Move to end of previous line
            if ((cursor_row > 0)); then
                ((cursor_row--))
                cursor_col=${#lines[$cursor_row]}
            fi
            return
        fi
        # Skip spaces
        while ((cursor_col > 0)) && [[ "${line:$((cursor_col - 1)):1}" == " " ]]; do
            ((cursor_col--))
        done
        # Skip word characters
        while ((cursor_col > 0)) && [[ "${line:$((cursor_col - 1)):1}" != " " ]]; do
            ((cursor_col--))
        done
    }

    # Move word right (Ctrl+Right)
    _tm_word_right() {
        local line="${lines[$cursor_row]}"
        local line_len=${#line}
        if ((cursor_col >= line_len)); then
            # Move to start of next line
            if ((cursor_row < ${#lines[@]} - 1)); then
                ((cursor_row++))
                cursor_col=0
            fi
            return
        fi
        # Skip word characters
        while ((cursor_col < line_len)) && [[ "${line:$cursor_col:1}" != " " ]]; do
            ((cursor_col++))
        done
        # Skip spaces
        while ((cursor_col < line_len)) && [[ "${line:$cursor_col:1}" == " " ]]; do
            ((cursor_col++))
        done
    }

    # === Setup ===
    old_stty=$(stty -g </dev/tty)
    printf "\033]0;typeMode\007" >/dev/tty  # Set title
    printf "\033[2J\033[H" >/dev/tty         # Clear screen
    stty raw -echo -isig </dev/tty

    # === Main Loop ===
    while true; do
        local char
        char=$(_tm_read_char)

        case "$char" in
            $'\x1b')  # Escape sequence
                local c2 c3
                IFS= read -rsn1 c2 </dev/tty
                if [[ "$c2" == "[" ]]; then
                    IFS= read -rsn1 c3 </dev/tty
                    case "$c3" in
                        A) _tm_move_up ;;      # Up arrow
                        B) _tm_move_down ;;    # Down arrow
                        C) _tm_move_right ;;   # Right arrow
                        D) _tm_move_left ;;    # Left arrow
                        H) _tm_home ;;         # Home
                        F) _tm_end ;;          # End
                        1)  # Extended sequences (Ctrl+arrows, Home, etc.)
                            local rest
                            IFS= read -rsn1 rest </dev/tty
                            case "$rest" in
                                ";")
                                    local mod seq
                                    IFS= read -rsn2 seq </dev/tty
                                    case "$seq" in
                                        5A) # Ctrl+Up - previous history
                                            if ((hist_idx > 0)); then
                                                ((hist_idx--))
                                                _tm_set_text "${history[$hist_idx]}"
                                            fi
                                            ;;
                                        5B) # Ctrl+Down - next history
                                            if ((hist_idx < ${#history[@]})); then
                                                ((hist_idx++))
                                                if ((hist_idx < ${#history[@]})); then
                                                    _tm_set_text "${history[$hist_idx]}"
                                                else
                                                    lines=("")
                                                    cursor_row=0
                                                    cursor_col=0
                                                fi
                                            fi
                                            ;;
                                        5C) _tm_word_right ;;  # Ctrl+Right
                                        5D) _tm_word_left ;;   # Ctrl+Left
                                    esac
                                    ;;
                                "~") _tm_home ;;  # Home (alternate)
                            esac
                            ;;
                        3)  # Delete key
                            IFS= read -rsn1 rest </dev/tty
                            if [[ "$rest" == "~" ]]; then
                                _tm_delete
                            fi
                            ;;
                        4)  # End (alternate)
                            IFS= read -rsn1 rest </dev/tty
                            if [[ "$rest" == "~" ]]; then
                                _tm_end
                            fi
                            ;;
                    esac
                fi
                _tm_render
                ;;
            $'\x18')  # Ctrl+X - copy, save to history, and clear
                local text
                text=$(_tm_get_text)
                if [[ -n "$text" ]]; then
                    printf '%s' "$text" | xclip -selection clipboard
                    history+=("$text")
                    hist_idx=${#history[@]}
                fi
                lines=("")
                cursor_row=0
                cursor_col=0
                _tm_render
                ;;
            $'\x04')  # Ctrl+D - exit
                stty "$old_stty" </dev/tty
                printf "\033]0;\007" >/dev/tty  # Reset title
                printf "\r\n" >/dev/tty
                return 0
                ;;
            $'\x15')  # Ctrl+U - clear all
                lines=("")
                cursor_row=0
                cursor_col=0
                _tm_render
                ;;
            $'\x7f'|$'\x08')  # Backspace
                _tm_backspace
                _tm_render
                ;;
            ''|$'\r'|$'\n')  # Enter - newline
                _tm_newline
                _tm_render
                ;;
            *)
                # Regular character - insert at cursor
                if [[ -n "$char" ]]; then
                    _tm_insert_char "$char"
                    _tm_render
                fi
                ;;
        esac
    done
}
export -f typeMode
alias tm='typeMode'
