#!/usr/bin/env bash

# Start and end colors in 256-color codes
start_color=34   # dark green
end_color=39     # light blue

# Get current working directory
dir="${PWD/#$HOME/~}"  # optional: shorten home to ~

# Length of string
len=${#dir}

# Output buffer
prompt=""

# Add terminal title
prompt+="\[\e]0;${PWD}\a\]"

# Loop through each character and assign interpolated color
for ((i=0; i<len; i++)); do
    # Calculate color index between start_color and end_color
    color=$((start_color + (i * (end_color - start_color) / (len - 1))))
    char="${dir:$i:1}"
    prompt+="\[\033[38;5;${color}m\]$char"
done

# Reset color
prompt+="\[\033[00m\]"

# Output
echo "$prompt/"
