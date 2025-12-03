firstTime(){
    # local AUTOMATE_LINUX_DAEMON_IN_FILE="${AUTOMATE_LINUX_DATA_DIR}relay_${AUTOMATE_LINUX_TTY_NUMBER}_in"
    # local AUTOMATE_LINUX_DAEMON_OUT_FILE="${AUTOMATE_LINUX_DATA_DIR}relay_${AUTOMATE_LINUX_TTY_NUMBER}_out"
    # rm -f "$AUTOMATE_LINUX_DAEMON_IN_FILE" "$AUTOMATE_LINUX_DAEMON_OUT_FILE" 2>/dev/null
    # mkfifo "$AUTOMATE_LINUX_DAEMON_IN_FILE" "$AUTOMATE_LINUX_DAEMON_OUT_FILE" 2>/dev/null
    # (
    #     exec 3<>"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null || exit 1
    #     while true; do
    #         stdbuf -o0 -i0 cat "$AUTOMATE_LINUX_DAEMON_IN_FILE" >&3
    #         stdbuf -o0 -i0 cat <&3 > "$AUTOMATE_LINUX_DAEMON_OUT_FILE"
    #     done
    # ) &
    # export AUTOMATE_LINUX_RELAY_PID=$!
    # sleep 0.2
    # exec {AUTOMATE_LINUX_DAEMON_IN_FD}>"$AUTOMATE_LINUX_DAEMON_IN_FILE"
    # exec {AUTOMATE_LINUX_DAEMON_OUT_FD}<"$AUTOMATE_LINUX_DAEMON_OUT_FILE"
    # trap "rm -f '$AUTOMATE_LINUX_DAEMON_IN_FILE' '$AUTOMATE_LINUX_DAEMON_OUT_FILE' 2>/dev/null; kill $AUTOMATE_LINUX_RELAY_PID 2>/dev/null" HUP
    exec {AUTOMATE_LINUX_DAEMON_FD}<> >(socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null)
    export AUTOMATE_LINUX_DAEMON_FD
}
firstTime
unset firstTime