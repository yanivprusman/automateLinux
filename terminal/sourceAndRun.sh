#!/bin/bash
daemonAlternative() {
    local transport="fd"
    if [[ "$1" == "--socat" || "$1" == "--nc" ]]; then
        transport="${1#--}"
        shift
    fi

    local formatOutput="false"
    local formatOutputTrueFunctions=("" "--help" "showDB" "showTerminalInstance" \
        "showAllTerminalInstances" "showEntriesByPrefix" "printDirHistory" "setKeyboard")

    if [[ " ${formatOutputTrueFunctions[*]} " == *" $1 "* ]]; then
        formatOutput="true"
    fi

    local args=()
    for arg in "$@"; do
        if [[ "$arg" == "--json" ]]; then
            formatOutput="true"
        else
            args+=("$arg")
        fi
    done

    local COMMAND="${args[0]}" json reply key value
    json="{\"command\":\"$COMMAND\",\"tty\":$AUTOMATE_LINUX_TTY_NUMBER"
    for ((i=1; i<${#args[@]}; i++)); do
        key="${args[$i]%%=*}"
        value="${args[$i]#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"

    if [[ "$transport" != "fd" ]]; then
        if [[ "$transport" == "nc" ]]; then
            reply="$(printf '%s\n' "$json" | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH")"
        else
            reply="$(printf '%s\n' "$json" | socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null)"
        fi
    else
        if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ] ||
           ! { true >&"$AUTOMATE_LINUX_DAEMON_FD_IN"; } 2>/dev/null ||
           ! { true <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"; } 2>/dev/null; then
            AUTOMATE_LINUX_DAEMON_PID=$(${AUTOMATE_LINUX_DAEMON_DIR}getDaemonPID.sh) && \
            coproc DAEMON_COPROC { socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null; } && \
            export AUTOMATE_LINUX_DAEMON_FD_IN=${DAEMON_COPROC[1]} && \
            export AUTOMATE_LINUX_DAEMON_FD_OUT=${DAEMON_COPROC[0]}
        fi

        if [[ "$COMMAND" == closedTty ]]; then
            printf '%s\n' "$json" | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH"
            return 0
        fi

        printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_FD_IN"
        read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"
    fi

    if [[ "$formatOutput" == "true" ]]; then
        printf '%s' "$reply" | awk '{gsub(/\\n/,"\n"); printf "%s", $0}
            END {if (substr($0,length,1)!="\n") printf "\n"}'
    else
        printf '%s' "$reply"
        [[ -n "$reply" ]] && printf '\n'
    fi
}
# export -f daemonAlternative

localFunction(){
    local theRealPath=$(daemonAlternative --socat getFile fileName=theRealPath.sh)
    local script_name=$(basename ${SUDO_COMMAND%% *} )
    if [[ "$script_name" == "print" ]]; then
        # source ../functions/printDir.sh
        . $(theRealPath ../functions/printDir.sh )
        print $@
    # else
    #     :
    #     . $(theRealPath "../functions/$script_name.sh" )
    #     "$script_name" $@
    fi
}
localFunction "$@"
unset localFunction
        # theRealPath ../functions/$script_name.sh
        # theRealPath ../functions/printDir.sh
        # echo asdf

# sourceAndRun() {
#     local theRealPath script_name script_path base_dir
#     theRealPath=$(daemonAlternative --socat getFile fileName=theRealPath.sh)
#     if [[ -n "$SUDO_COMMAND" ]]; then
#         script_name="$(basename "${SUDO_COMMAND%% *}")"
#     else
#         script_name="$(basename "$0")"
#     fi
#     if [[ "$script_name" == "theRealPath" ]]; then
#         echo b4 sourcing _theRealPath
#         . _theRealPath
#         echo after sourcing _theRealPath
#     fi
#     echo "script_name: $script_name" >&2
#         return 0
#     # Resolve base directory relative to this script, not CWD
#     base_dir="$(dirname "$(theRealPath "${BASH_SOURCE[0]}")")"
#     echo "base_dir: $base_dir" >&2

#     # if [[ "$script_name" == "print" ]]; then
#     #     script_path="$(theRealPath "$base_dir/../functions/printDir.sh")"
#     #     . "$script_path"
#     #     print "$@"
#     # else
#     #     script_path="$(theRealPath "$base_dir/../functions/$script_name.sh")"
#     #     . "$script_path"
#     #     "$script_name" "$@"
#     # fi
# }



sourceAndRun "$@"
unset sourceAndRun
