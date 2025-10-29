
getRealPath() { # sudo $(getRealPath /relative/path/to/script.sh)
    local relativePath="$1"
    if [ -z "$relativePath" ]; then
        echo "Error: No path provided" >&2
        return 1
    fi
    relativePath="${relativePath#/}"
    # If called from a script, use the script's directory
    if [ "${BASH_SOURCE[1]}" != "" ]; then
        echo "$(realpath "$(dirname "$(realpath "${BASH_SOURCE[1]}")")/${relativePath}")"
    else
        # If called from command line, use current directory
        echo "$(realpath "${PWD}/${relativePath}")"
    fi
}
export -f getRealPath
