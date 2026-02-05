# System and process utilities

runSingleton() {
    local SCRIPT="$1"
    if [ -f "$SCRIPT" ] && ! pgrep -f "$SCRIPT" > /dev/null; then
        "$SCRIPT" &
    else
        :
    fi
}
export -f runSingleton

killAllJobs() {
    for job in $(jobs -p); do
        kill -9 $job 2>/dev/null
    done
}
export -f killAllJobs

status(){
    systemctl status "$@"
}
export -f status

start(){
    systemctl start "$@"
}
export -f start

stop(){
    systemctl stop "$@"
}
export -f stop

reload(){
    systemctl daemon-reload
}
export -f reload

toSymbolic() {
    local oct="$1"
    python3 -c "import stat; print(stat.filemode(int('$oct', 8)))"
}
export -f toSymbolic

logOut() {
    # echo "Logging out of current session..."
    # gnome-session-quit --logout --no-prompt || loginctl terminate-session ${XDG_SESSION_ID:-self}
    loginctl terminate-session 21
}
export -f logOut

shutDown() {
    systemctl poweroff
}
export -f shutDown

_shutdownin_parse_minutes() {
    local input="$1"
    if [[ "$input" =~ ^([0-9]+)[hH]$ ]]; then
        echo $(( ${BASH_REMATCH[1]} * 60 ))
    elif [[ "$input" =~ ^([0-9]+)[mM]?$ ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        return 1
    fi
}

_shutdownin_format_duration() {
    local minutes=$1
    if (( minutes >= 60 )); then
        local h=$((minutes / 60)) m=$((minutes % 60))
        if (( m > 0 )); then echo "${h}h ${m}m"; else echo "${h}h"; fi
    else
        echo "${minutes}m"
    fi
}

shutDownIn() {
    local minutes

    if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
        echo "Usage: shutDownIn [TIME]"
        echo ""
        echo "Schedule a system shutdown. Without arguments, shows an interactive menu."
        echo ""
        echo "TIME formats:"
        echo "  30       30 minutes (bare number = minutes)"
        echo "  90m      90 minutes"
        echo "  2h       2 hours"
        echo ""
        echo "Examples:"
        echo "  shutDownIn           Interactive menu"
        echo "  shutDownIn 30        Shutdown in 30 minutes"
        echo "  shutDownIn 2h        Shutdown in 2 hours"
        echo ""
        echo "Cancel a scheduled shutdown: sudo shutdown -c"
        return 0
    fi

    if [[ -n "${1:-}" ]]; then
        minutes=$(_shutdownin_parse_minutes "$1") || { echo "Invalid format: $1 (use e.g. 30, 90m, 2h)"; return 1; }
    else
        echo "Schedule a shutdown:"
        echo ""
        echo "  1) 30 minutes"
        echo "  2) 1 hour"
        echo "  3) 1.5 hours"
        echo "  4) 2 hours"
        echo "  5) 3 hours"
        echo "  6) Custom"
        echo ""
        local choice
        read -p "Choose [1-6]: " choice
        case "$choice" in
            1) minutes=30 ;;
            2) minutes=60 ;;
            3) minutes=90 ;;
            4) minutes=120 ;;
            5) minutes=180 ;;
            6)
                local custom
                read -p "Enter time (e.g. 45, 90m, 2h): " custom
                minutes=$(_shutdownin_parse_minutes "$custom") || { echo "Invalid format: $custom"; return 1; }
                ;;
            *) echo "Invalid choice."; return 1 ;;
        esac
    fi

    local shutdown_time
    shutdown_time=$(date -d "+${minutes} minutes" '+%H:%M')
    local display
    display=$(_shutdownin_format_duration "$minutes")

    echo ""
    read -p "Shutdown at $shutdown_time ($display from now). Proceed? [Y/n]: " confirm
    if [[ "$confirm" =~ ^[Nn] ]]; then
        echo "Cancelled."
        return 0
    fi

    sudo shutdown +$minutes
    echo "Shutdown scheduled. Cancel with: sudo shutdown -c"
}
export -f shutDownIn

setDesktopBackground(){
    gsettings set org.gnome.desktop.background picture-uri 'file:///usr/share/images/desktop-base/desktop-background.xml'
}
export -f setDesktopBackground

disableScreenLock(){
    gsettings set org.gnome.desktop.screensaver lock-enabled false
    gsettings set org.gnome.desktop.screensaver lock-delay 0
    gsettings set org.gnome.desktop.session idle-delay 0
}
export -f disableScreenLock

fixAutoLinuxPerms() {
    local target="${1:-/opt/automateLinux}"
    echo "Fixing permissions on $target..."
    sudo chown -R root:coding "$target"
    sudo chmod -R g+w "$target"
    sudo find "$target" -type d -exec chmod g+s {} \;
    echo "Done. Ownership: root:coding, group-writable, setgid on dirs."
}
export -f fixAutoLinuxPerms

sudoVisudo() {
    # Use script to allocate a PTY, bypassing terminal capture issues
    script -q -c 'sudo visudo' /dev/null
}
export -f sudoVisudo

ubuntuVersion() {
    lsb_release -d | cut -f2
}
export -f ubuntuVersion