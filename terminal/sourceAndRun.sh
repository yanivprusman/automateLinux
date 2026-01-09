#!/bin/bash
daemonAlternative() {
    local transport="fd"
    if [[ "$1" == "--socat" || "$1" == "--nc" ]]; then
        transport="${1#--}"
        shift
    fi

    local formatOutput="false"
    local formatOutputTrueFunctions=("" "--help" "showDB" "showTerminalInstance" 
        "showAllTerminalInstances" "showEntriesByPrefix" "printDirHistory" "enableKeyboard" "disableKeyboard")

    if [[ " ${formatOutputTrueFunctions[*]} " == "* $1 *" ]]; then
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
    json="{\"command\":\"$COMMAND\",\"tty\":\"$AUTOMATE_LINUX_TTY_NUMBER\""
    for ((i=1; i<${#args[@]}; i++)); do
        key="${args[$i]%%=*}"
        value="${args[$i]#*=}"
        json+=(",\"$key\":\"$value\"")
    done
    json+="}"

    if [[ "$transport" != "fd" ]]; then
        if [[ "$transport" == "nc" ]]; then
            reply="$(printf '%s\n' "$json" | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH")"
        else
            reply="$(printf '%s\n' "$json" | socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null)"
        fi
    else
        if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ] || \
           ! { true >&"$AUTOMATE_LINUX_DAEMON_FD_IN"; } 2>/dev/null || \
           ! { true <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"; } 2>/dev/null;
then
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
        printf '%s' "$reply" | awk '{gsub(/\\n/, "\n"); printf "%s", $0}
            END {if (substr($0,length,1)!="\n") printf "\n"}'
    else
        printf '%s' "$reply"
        [[ -n "$reply" ]] && printf '\n'
    fi
}
# export -f daemonAlternative

getCallType() {
    local bottom="${FUNCNAME[-1]}"
    if [[ "$bottom" == "main" ]]; then
        printf "%s\n" "subprocessed"
    elif [[ "$bottom" == "source" ]]; then
        printf "%s\n" "sourced"
    else
        printf "%s\n" "terminal"
    fi
}

sourceAndRun() {
    local script_name
    local originalCallType="$(getCallType)"
    local debug_flag=

    # Check for -debug flag in the arguments to sourceAndRun
    local all_args=("$@")
    local pass_args=()
    for arg in "${all_args[@]}"; do
        if [[ "$arg" == "-debug" ]]; then
            debug_flag="-debug"
        fi
        pass_args+=("$arg")
    done

    if [[ -n "$SUDO_COMMAND" ]]; then
        script_name="$(basename "${SUDO_COMMAND%% *}")"
    else
        script_name="$(basename "$0")"
    fi

    # Determine the directory of this script, which acts as the base for sourcing
    local base_dir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

    if [[ "$script_name" == "theRealPath" ]]; then
        # Execute the theRealPath.sh script as a subprocess, passing the original call type
        # and the sudoCommand if applicable
        if [[ -n "$SUDO_COMMAND" ]]; then
            sudo_command_path=$(which "${SUDO_COMMAND%% *}" 2>/dev/null)
            "$base_dir/theRealPath.sh" "$debug_flag" -originalCallType "$originalCallType" -sudoCommand "$sudo_command_path" "${pass_args[@]}"
        else
            "$base_dir/theRealPath.sh" "$debug_flag" -originalCallType "$originalCallType" "${pass_args[@]}"
        fi
    elif [[ "$script_name" == "print" ]]; then
        # Example for other scripts: source and call a "print" function
        . "$base_dir/functions/printDir.sh"
        print "$@"
    else
        echo "Error: Unknown script name '$script_name'" >&2
        exit 1
    fi
}

# Execute the sourceAndRun function with all arguments passed to this script
sourceAndRun "$@"
