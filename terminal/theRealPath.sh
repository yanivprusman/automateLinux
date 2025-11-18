#!/bin/bash
# added shebang so when run with sudo and uses different shell it knows to use bash
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

theRealPath() {
    local callType="$(getCallType)"
    local callingScript target
    if [[ $1 == "-sudoCommand" ]]; then
        shift
        local script="$1"
        shift
    fi    
    if [[ $1 == "/" ]]; then
        printf "/\n"
        return 0
    fi
    if [[ $1 == /* ]]; then
        target="$(realpath "$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printf "%s\n" "$1"
            return 1
        fi
    elif [[ "$callType" ==  "$CALL_TYPE_TERMINAL" ]]; then
        target="$(realpath "${PWD}/$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printf "%s\n" "${PWD}/$1"
            return 1
        fi
    elif [[ "$callType" == "$CALL_TYPE_SUBPROCESSED" ]] || [[ "$callType" == "$CALL_TYPE_SOURCED" ]]; then
        callingScript="$(realpath "${BASH_SOURCE[1]}" 2>/dev/null)"
        if [[ ! -z "$script" ]]; then
            callingScript="$script"
        fi 
        if [[ -z "$1" ]]; then
            printf "%s\n" "$callingScript"
        else
            target="$(realpath "$(dirname "$callingScript")/$1")"
            if ! printFileOrDirRealPath "$target"; then
                # printf "%s\n" "$(dirname ${BASH_SOURCE[1]})/$1" TODO fix to this line
                printf "%s\n" "$(dirname "$callingScript")/$1"
                return 1
            fi
        fi
    fi
}
export -f theRealPath

printTheRealPath() {
    local callType="$(getCallType)"
    local callingScript target
    if [[ $1 == "-sudoCommand" ]]; then
        shift
        local script="$1"
        shift
    fi    
    if [[ $1 == "/" ]]; then
        printf "/\n"
        return 0
    fi
    if [[ $1 == /* ]]; then
        target="$(realpath "$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printf "%s\n" "$1"
            return 1
        fi
    elif [[ "$callType" ==  "$CALL_TYPE_TERMINAL" ]]; then
        echo test1
        target="$(realpath "${PWD}/$1" 2>/dev/null)"
        if ! printFileOrDirRealPath "$target"; then
            printf "%s\n" "${PWD}/$1"
            return 1
        fi
    elif [[ "$callType" == "$CALL_TYPE_SUBPROCESSED" ]] || [[ "$callType" == "$CALL_TYPE_SOURCED" ]]; then
        # callingScript="$(realpath "${BASH_SOURCE[1]}$1" 2>/dev/null)"
        callingScript="$(realpath "${BASH_SOURCE[1]}" 2>/dev/null)"
        echo $callingScript
        if [[ ! -z "$script" ]]; then
            callingScript="$script"
        fi 
        if [[ -z "$1" ]]; then
            printf "%s\n" "$callingScript"
        else
            target="$(realpath "$(dirname "$callingScript")/$1")"
            if ! printFileOrDirRealPath "$target"; then
                # printf "%s\n" "$(dirname "$callingScript")/$1"
                printf "%s\n" "$(dirname ${BASH_SOURCE[1]})/$1"
                return 1
            fi
        fi
    fi
}
export -f printTheRealPath

if a=$(getCallType) && [ "$a" == "$CALL_TYPE_SUBPROCESSED" ]; then
    # echo debug log test if called not from sudo
    theRealPath -sudoCommand "$(realpath $SUDO_COMMAND)" "$@"
fi


# FUNCNAME
#         An  array  variable containing the names of all shell functions currently in the execution call stack.  The element with index 0
#         is the name of any currently-executing shell function.  The bottom-most element (the one with  the  highest  index)  is  "main".
#         This variable exists only when a shell function is executing.  Assignments to FUNCNAME have no effect.  If FUNCNAME is unset, it
#         loses its special properties, even if it is subsequently reset.
#         This  variable can be used with BASH_LINENO and BASH_SOURCE.  Each element of FUNCNAME has corresponding elements in BASH_LINENO
#         and BASH_SOURCE to describe the call stack.  For instance, ${FUNCNAME[$i]} was called from the file ${BASH_SOURCE[$i+1]} at line
#         number ${BASH_LINENO[$i]}.  The caller builtin displays the current call stack using this information.
