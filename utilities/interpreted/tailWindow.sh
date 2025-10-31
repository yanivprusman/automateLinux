#!/bin/bash

# Get terminal info
TERM_INFO=$(termcontrol --buffer)
ROWS=$(echo "$TERM_INFO" | grep "Rows:" | cut -d' ' -f4)
COLS=$(echo "$TERM_INFO" | grep "Columns:" | cut -d' ' -f4)

# Calculate window position (bottom quarter of screen)
START_ROW=$((ROWS * 3 / 4))
HEIGHT=$((ROWS - START_ROW))
BORDER_COLOR="4" # Blue
TEXT_COLOR="7"   # White
BG_COLOR="0"     # Black

# Draw top border
termcontrol --set-range $START_ROW 0 "+" --color $BORDER_COLOR $BG_COLOR
termcontrol --set-range $START_ROW 1 $(printf '=%.0s' $(seq 1 $((COLS-2)))) --color $BORDER_COLOR $BG_COLOR
termcontrol --set-range $START_ROW $((COLS-1)) "+" --color $BORDER_COLOR $BG_COLOR

# Draw side borders
for ((r=START_ROW+1; r<ROWS-1; r++)); do
    termcontrol --set $r 0 "|" --color $BORDER_COLOR $BG_COLOR
    termcontrol --set $r $((COLS-1)) "|" --color $BORDER_COLOR $BG_COLOR
done

# Draw bottom border
termcontrol --set-range $((ROWS-1)) 0 "+" --color $BORDER_COLOR $BG_COLOR
termcontrol --set-range $((ROWS-1)) 1 $(printf '=%.0s' $(seq 1 $((COLS-2)))) --color $BORDER_COLOR $BG_COLOR
termcontrol --set-range $((ROWS-1)) $((COLS-1)) "+" --color $BORDER_COLOR $BG_COLOR

# Save cursor and enable raw mode
termcontrol --cursor save
termcontrol --raw on

# Function to display text in the window
display_text() {
    local text="$1"
    local row=$((START_ROW + 1))
    local max_width=$((COLS - 2))
    
    # Clear previous content
    for ((r=row; r<ROWS-1; r++)); do
        termcontrol --set-range $r 1 "$(printf ' %.0s' $(seq 1 $((COLS-2))))" --color $TEXT_COLOR $BG_COLOR
    done
    
    # Display new content
    while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ $row -lt $((ROWS-1)) ]]; then
            termcontrol --set-range $row 1 "${line:0:$max_width}" --color $TEXT_COLOR $BG_COLOR
            ((row++))
        fi
    done <<< "$text"
}

# Read from stdin and update window
while IFS= read -r line || [[ -n "$line" ]]; do
    content+="$line"$'\n'
    # Keep only last N lines that fit in the window
    content=$(echo "$content" | tail -n $((HEIGHT-2)))
    display_text "$content"
done

# Cleanup
termcontrol --raw off
termcontrol --cursor restore
termcontrol --reset