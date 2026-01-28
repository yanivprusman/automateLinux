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