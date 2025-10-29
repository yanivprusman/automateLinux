alias mk='mkdir'
alias boot='history -a && sudo reboot'
alias pkill='sudo pkill -9'
alias pkillev='sudo pkill -9 evsieve'
alias evsievep='sudo evsieve --input /dev/input/event* --print format=direct'
alias 1restart='restartCorsairKeyBoardLogiMouseService.sh reset'
alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'

# sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/_sudoStop.sh")

scriptPath() {
    # First argument is the relative path to resolve
    local relativePath="$1"
    # Remove leading slash if present
    relativePath="${relativePath#/}"
    echo "$(realpath "$(dirname "$(realpath "${BASH_SOURCE[1]}")")/${relativePath}")"
}
export -f scriptPath

# Usage example:
# sudo $(scriptPath _sudoStop.sh)