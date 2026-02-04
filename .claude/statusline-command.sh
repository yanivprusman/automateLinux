#!/bin/bash

# Read JSON input from stdin
input=$(cat)

# Extract values from JSON
model=$(echo "$input" | jq -r '.model.display_name')
cwd=$(echo "$input" | jq -r '.workspace.current_dir')
used_pct=$(echo "$input" | jq -r '.context_window.used_percentage // empty')
transcript_path=$(echo "$input" | jq -r '.transcript_path')

# Calculate session duration
if [ -f "$transcript_path" ]; then
    start_time=$(stat -c %Y "$transcript_path" 2>/dev/null)
    current_time=$(date +%s)
    duration_seconds=$((current_time - start_time))

    if [ $duration_seconds -lt 60 ]; then
        duration="${duration_seconds}s"
    elif [ $duration_seconds -lt 3600 ]; then
        minutes=$((duration_seconds / 60))
        duration="${minutes}m"
    else
        hours=$((duration_seconds / 3600))
        minutes=$(((duration_seconds % 3600) / 60))
        duration="${hours}h${minutes}m"
    fi
else
    duration="0s"
fi

# Get cost from JSON (provided by Claude Code)
total_cost=$(echo "$input" | jq -r '.cost.total_cost_usd // 0')
cost_display=$(printf "\$%.3f" $total_cost)

# Get lines added/removed from JSON (provided by Claude Code)
lines_added=$(echo "$input" | jq -r '.cost.total_lines_added // 0')
lines_removed=$(echo "$input" | jq -r '.cost.total_lines_removed // 0')

# Format context usage
if [ -n "$used_pct" ]; then
    ctx_display=$(printf "%.1f%%" "$used_pct")
else
    ctx_display="0.0%"
fi

# Color codes
CYAN='\033[36m'
GREEN='\033[32m'
YELLOW='\033[33m'
BLUE='\033[34m'
MAGENTA='\033[35m'
RED='\033[31m'
RESET='\033[0m'

# Build status line with colors
printf "${CYAN}%s${RESET} | ${BLUE}%s${RESET} | ${YELLOW}ctx:%s${RESET} | ${MAGENTA}%s %s${RESET} | ${GREEN}+%d${RESET}/${RED}-%d${RESET}" \
    "$model" \
    "$cwd" \
    "$ctx_display" \
    "$cost_display" \
    "$duration" \
    "$lines_added" \
    "$lines_removed"
