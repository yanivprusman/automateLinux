# # Daemon and IPC utilities

# daemon() {
#     if [ -z "$AUTOMATE_LINUX_DAEMON_IN_FD" ] || [ -z "$AUTOMATE_LINUX_DAEMON_OUT_FD" ]; then
#         return 1
#     fi
    
#     local COMMAND="$1" json key value reply
#     shift
#     json="{\"command\":\"$COMMAND\""
#     json+=",\"tty\":\"$AUTOMATE_LINUX_TTY_NUMBER\""
#     for arg in "$@"; do
#         key="${arg%%=*}"
#         value="${arg#*=}"
#         json+=",\"$key\":\"$value\""
#     done
#     json+="}"
    
#     # Write to FD and read response (with timeout)
#     printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_IN_FD"
#     read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_OUT_FD"
#     printf '%s\n' "$reply"
# }
# export -f daemon

# d(){
#     echo "in d function"
#     daemon "$@"
# }
# export -f d

# initCoproc() {
#     if [[ -n "${DAEMON_CO_PID-}" ]] && kill -0 "$DAEMON_CO_PID" 2>/dev/null; then
#         exec {DAEMON_CO[0]}<&- {DAEMON_CO[1]}>&- 2>/dev/null
#         sleep 0.1
#         kill -9 "$DAEMON_CO_PID" 2>/dev/null
#         wait "$DAEMON_CO_PID" 2>/dev/null || true
#     fi
#     unset DAEMON_CO DAEMON_CO_PID
#     coproc DAEMON_CO { nc -U "$AUTOMATE_LINUX_SOCKET_PATH"; }
#     DAEMON_CO_PID=$!
# }
# export -f initCoproc

# firstFreeFd() {
#     local fd
#     for fd in {3..255}; do
#         if ! { : >&"$fd"; } >/dev/null 2>&1; then 
#             printf '%s\n' "$fd"
#             return 0
#         fi
#     done
#     return 1
# }
# export -f firstFreeFd

daemon() {
    if [ -z "$AUTOMATE_LINUX_DAEMON_FD" ]; then
        return 1
    fi
    local COMMAND="$1" json key value reply
    shift
    json="{\"command\":\"$COMMAND\""
    json+=",\"tty\":$AUTOMATE_LINUX_TTY_NUMBER"
    for arg in "$@"; do
        key="${arg%%=*}"
        value="${arg#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"
    echo "in daemon function, sending json: $json" >&2
    printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_FD"
    read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_FD"
    printf '%s\n' "$reply"
}
export -f daemon

d(){
    daemon "$@"
}
export -f d