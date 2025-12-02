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

reload(){
    systemctl daemon-reload
}
export -f reload

toSymbolic() {
    local oct="$1"
    python3 -c "import stat; print(stat.filemode(int('$oct', 8)))"
}
export -f toSymbolic

