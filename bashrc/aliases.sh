alias mk='mkdir'
alias boot='history -a && sudo reboot'
alias pkill='sudo pkill -9'
alias pkillev='sudo pkill -9 evsieve'
alias evsievep='sudo evsieve --input /dev/input/event* --print format=direct'
alias 1restart='restartCorsairKeyBoardLogiMouseService.sh reset'
alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'
#replacing
# sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/_sudoStop.sh")
#with
# sudo $(runScriptRelative _sudoStop.sh)

runScriptRelative() {
    # First argument is the relative path to resolve
    local relativePath="$1"
    if [ -z "$relativePath" ]; then
        echo "Error: No path provided" >&2
        return 1
    fi
    
    # Remove leading slash if present
    relativePath="${relativePath#/}"
    
    # If called from a script, use the script's directory
    if [ "${BASH_SOURCE[1]}" != "" ]; then
        echo "$(realpath "$(dirname "$(realpath "${BASH_SOURCE[1]}")")/${relativePath}")"
    else
        # If called from command line, use current directory
        echo "$(realpath "${PWD}/${relativePath}")"
    fi
}
export -f runScriptRelative

# Shorter alias for runScriptRelative
alias rsr='runScriptRelative'
# Legacy alias to maintain compatibility
alias getRealPath='runScriptRelative'

# Usage examples:
# From scripts:   sudo $(runScriptRelative _sudoStop.sh)
# Short version:  sudo $(rsr _sudoStop.sh)
# Command line:   sudo $(getRealPath some/path)