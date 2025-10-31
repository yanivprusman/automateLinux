#!/usr/bin/env bash
cleanup() {
    tput sgr0     # reset text formatting
    exit 0
}

trap cleanup QUIT

# Function to display usage information
show_help() {
    echo "Usage: tailWindow [-h|--help] [-s|--size NUM] FILENAME"
    echo "Monitor a file with a fixed-size sliding window display."
    echo ""
    echo "Options:"
    echo "  -h, --help       Show this help message"
    echo "  -s, --size NUM   Set window size (number of lines), default is 5"
    echo ""
    echo "Example:"
    echo "  tailWindow -s 10 /var/log/syslog"
    exit 0
}

# Function to validate if input is a positive integer
is_positive_integer() {
    [[ "$1" =~ ^[0-9]+$ ]] && [ "$1" -gt 0 ]
}

# Default values
window_size=5
file=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -s|--size)
            if [ -z "$2" ] || ! is_positive_integer "$2"; then
                echo "Error: Window size must be a positive integer"
                exit 1
            fi
            window_size="$2"
            shift 2
            ;;
        *)
            if [ -z "$file" ]; then
                file="$1"
            else
                echo "Error: Only one file can be monitored at a time"
                exit 1
            fi
            shift
            ;;
    esac
done

# Check if file is provided
if [ -z "$file" ]; then
    # echo "Error: No file specified"
    show_help
fi

# Check if file exists and is readable
if [ ! -r "$file" ]; then
    echo "Error: File '$file' does not exist or is not readable"
    exit 1
fi

# Monitor file with the specified window size
tail -f "$file" 2>/dev/null | awk -v window_size="$window_size" '
{
    # Shift lines up
    for(i=1; i<window_size; i++) lines[i] = lines[i+1]
    lines[window_size] = $0

    # Determine how many lines to print (don’t exceed NR or window_size)
    count = (NR < window_size) ? NR : window_size

    # Move cursor up only after the first line
    if (NR > 1) printf "\033[" count "A"

    # Print up to count lines
    for(i=window_size - count + 1; i<=window_size; i++) {
        if (i in lines) printf "\r\033[K%s\n", lines[i]
    }

    fflush()
}'