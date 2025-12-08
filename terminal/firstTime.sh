firstTime(){
    trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
    trap ". $AUTOMATE_LINUX_TRAP_EXIT_FILE" EXIT
    trap ". $AUTOMATE_LINUX_TRAP_HUP_FILE" HUP
    coproc DAEMON_COPROC { socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null; }
    export AUTOMATE_LINUX_DAEMON_FD_IN=${DAEMON_COPROC[1]}
    export AUTOMATE_LINUX_DAEMON_FD_OUT=${DAEMON_COPROC[0]}
    cd $(daemon openedTty)
}
firstTime
unset firstTime