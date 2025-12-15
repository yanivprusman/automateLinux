#!/bin/bash
# # added shebang so when run with sudo and uses different shell it knows to use bash
export CALL_TYPE_SUBPROCESSED="subprocessed"
export CALL_TYPE_SOURCED="sourced"
export CALL_TYPE_TERMINAL="terminal"

getCallType() {
    local bottom="${FUNCNAME[-1]}"
    if [[ "$bottom" == "main" ]]; then
        printf "%s\n" "$CALL_TYPE_SUBPROCESSED"
    elif [[ "$bottom" == "source" ]]; then
        printf "%s\n" "$CALL_TYPE_SOURCED"
    else
        printf "%s\n" "$CALL_TYPE_TERMINAL"
    fi
}
export -f getCallType

printFileOrDirRealPath() {
    local path="$1"
    if [[ -f "$path" ]]; then
        printf "%s\n" "$path"
        return 0
    elif [[ -d "$path" ]]; then
        printf "%s/\n" "$path"
        return 0
    fi
    return 1
}
export -f printFileOrDirRealPath

printDebug() {
    local debug=false
    if [[ $# -lt 1 ]]; then return; fi
    debug="$1"
    shift
    [[ $debug != true ]] && return
    while [[ $# -gt 0 ]]; do
        local name="$1"
        local value="$2"
        echo -e "${YELLOW}$name: $value${NC}" >&2
        shift 2
    done
}
export printDebug

theRealPath() {
    local debug= sudoCommand= args=() callingScript= target=
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -debug)
                debug=true
                shift
                ;;
            -sudoCommand)
                shift
                sudoCommand="$1"
                shift
                ;;
            *)
                args+=("$1")
                shift
                ;;
        esac
    done
    set -- "${args[@]}"
    local callType="$(getCallType)"
    printDebug $debug "call type" $callType
    if [[ -n "$sudoCommand" ]]; then
        printDebug $debug "sudo script" "$sudoCommand"
    fi    
    if [[ $1 == "-sudoCommand" ]]; then
        shift
        local script="$1"
        shift
        printDebug $debug "sudo script" "$script"
    fi    
    if [[ $1 == "/" ]]; then
        printf "/\n"
        return 0
    fi
    if [[ $1 == /* ]]; then
        target="$(realpath "$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printDebug $debug "realpath failed for" "$1"
            printf "%s\n" "$1"
            return 1
        fi
    elif [[ "$callType" ==  "$CALL_TYPE_TERMINAL" ]]; then
        target="$(realpath "${PWD}/$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printDebug $debug "realpath failed for" "${PWD}/$1"
            printf "%s\n" "${PWD}/$1"
            return 1
        fi
    elif [[ "$callType" == "$CALL_TYPE_SUBPROCESSED" ]] || [[ "$callType" == "$CALL_TYPE_SOURCED" ]]; then
        callingScript="$(realpath "${BASH_SOURCE[1]}" 2>/dev/null)"
        if [[ -n "$sudoCommand" ]]; then
            callingScript="$sudoCommand"
        fi 
        if [[ -z "$1" ]]; then
            printf "%s\n" "$callingScript"
        else
            target="$(realpath "$(dirname "$callingScript")/$1")"
            if ! printFileOrDirRealPath "$target"; then
                printDebug $debug "realpath failed for" "$(dirname "$callingScript")/$1"
                # printf "%s\n" "$(dirname ${BASH_SOURCE[1]})/$1" TODO fix to this line
                printf "%s\n" "$(dirname "$callingScript")/$1"
                return 1
            fi
        fi
    fi
}
export -f theRealPath

# if a=$(getCallType) && [ "$a" == "$CALL_TYPE_SUBPROCESSED" ]; then
#     sudoCommandWithoutParamaters=${SUDO_COMMAND%% *}
#     # echo command: $sudoCommandWithoutParamaters
#     # echo arguments: $@
#     # printDebug  $sudoCommandWithoutParamaters "sudo command without parameters" "$sudoCommandWithoutParamaters"
#     theRealPath -sudoCommand "$(realpath $sudoCommandWithoutParamaters)" "$@"
# fi
# unset a

# from mand bash:
# FUNCNAME
#         An  array  variable containing the names of all shell functions currently in the execution call stack.  The element with index 0
#         is the name of any currently-executing shell function.  The bottom-most element (the one with  the  highest  index)  is  "main".
#         This variable exists only when a shell function is executing.  Assignments to FUNCNAME have no effect.  If FUNCNAME is unset, it
#         loses its special properties, even if it is subsequently reset.
#         This  variable can be used with BASH_LINENO and BASH_SOURCE.  Each element of FUNCNAME has corresponding elements in BASH_LINENO
#         and BASH_SOURCE to describe the call stack.  For instance, ${FUNCNAME[$i]} was called from the file ${BASH_SOURCE[$i+1]} at line
#         number ${BASH_LINENO[$i]}.  The caller builtin displays the current call stack using this information.

