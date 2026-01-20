# # Show Claude Code usage stats
# # Uses ccusage if available, otherwise parses local JSONL files
# claudeUsage(){
#     local mode="${1:-blocks}"  # blocks, daily, monthly, session

#     # Check if ccusage is available
#     if command -v ccusage &>/dev/null || npx ccusage --version &>/dev/null 2>&1; then
#         npx ccusage "$mode"
#     else
#         # Fallback: parse local JSONL files directly
#         local claude_dir="$HOME/.claude/projects"

#         if [[ ! -d "$claude_dir" ]]; then
#             echo "No Claude usage data found at $claude_dir"
#             return 1
#         fi

#         echo "=== Claude Code Usage (last 24h) ==="
#         echo ""

#         local total_input=0
#         local total_output=0
#         local cutoff=$(date -d '24 hours ago' +%s 2>/dev/null || date -v-24H +%s)

#         # Find and parse JSONL files
#         while IFS= read -r file; do
#             while IFS= read -r line; do
#                 # Extract timestamp and check if within 24h
#                 local ts=$(echo "$line" | grep -oP '"timestamp":\s*"\K[^"]+' 2>/dev/null)
#                 if [[ -n "$ts" ]]; then
#                     local file_epoch=$(date -d "$ts" +%s 2>/dev/null)
#                     if [[ -n "$file_epoch" && "$file_epoch" -ge "$cutoff" ]]; then
#                         # Extract token counts
#                         local input=$(echo "$line" | grep -oP '"input_tokens":\s*\K[0-9]+' 2>/dev/null)
#                         local output=$(echo "$line" | grep -oP '"output_tokens":\s*\K[0-9]+' 2>/dev/null)
#                         total_input=$((total_input + ${input:-0}))
#                         total_output=$((total_output + ${output:-0}))
#                     fi
#                 fi
#             done < "$file"
#         done < <(find "$claude_dir" -name "*.jsonl" -type f 2>/dev/null)

#         local total=$((total_input + total_output))

#         printf "Input tokens:  %'d\n" "$total_input"
#         printf "Output tokens: %'d\n" "$total_output"
#         printf "Total tokens:  %'d\n" "$total"
#         echo ""
#         echo "Install ccusage for detailed reports: npm install -g ccusage"
#         echo "Or run: npx ccusage blocks"
#     fi
# }

claudeLimitReset(){
    local initial_datetime="${1:-19/01/26 06:00}"
    local cycle_seconds=$((5 * 60 * 60))  # 5 hours in seconds

    # Parse initial datetime (dd/mm/yy HH:MM)
    local day=$(echo "$initial_datetime" | cut -d'/' -f1)
    local month=$(echo "$initial_datetime" | cut -d'/' -f2)
    local year=$(echo "$initial_datetime" | cut -d'/' -f3 | cut -d' ' -f1)
    local time=$(echo "$initial_datetime" | cut -d' ' -f2)

    # Convert to full year
    if [[ ${#year} -eq 2 ]]; then
        year="20$year"
    fi

    # Convert initial datetime to epoch
    local initial_epoch=$(date -d "$year-$month-$day $time" +%s 2>/dev/null)
    if [[ -z "$initial_epoch" ]]; then
        echo "Error: Invalid date format. Use dd/mm/yy HH:MM"
        return 1
    fi

    local now_epoch=$(date +%s)

    # Calculate elapsed time since initial datetime
    local elapsed=$((now_epoch - initial_epoch))

    # Calculate completed cycles and position in current cycle
    local completed_cycles=$((elapsed / cycle_seconds))
    local position_in_cycle=$((elapsed % cycle_seconds))

    # Calculate remaining time in current cycle
    local remaining=$((cycle_seconds - position_in_cycle))

    # Calculate next reset time
    local next_reset_epoch=$((initial_epoch + (completed_cycles + 1) * cycle_seconds))
    local next_reset=$(date -d "@$next_reset_epoch" "+%H:%M")
    local next_reset_full=$(date -d "@$next_reset_epoch" "+%d/%m/%y %H:%M")

    # Format remaining time
    local remaining_hours=$((remaining / 3600))
    local remaining_minutes=$(((remaining % 3600) / 60))

    echo "Next reset: $next_reset_full"
    echo "Remaining:  ${remaining_hours}h ${remaining_minutes}m"
}
