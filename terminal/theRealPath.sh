#!/bin/bash
# added shebang so when run with sudo and uses different shell it knows to use bash
export CALL_TYPE_SUBPROCESSED="subprocessed"
export CALL_TYPE_SOURCED="sourced"
export CALL_TYPE_TERMINAL="terminal"

printFileOrDirRealPath() {
    local path="$1"
    if [[ -z "$path" ]]; then
        return 1
    fi
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
export -f printDebug


theRealPath() {
    local debug=false sudoCommand= originalCallType=
    local args=()
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
            -originalCallType) 
                shift
                originalCallType="$1"
                shift
                ;; 
            *) 
                args+=("$1")
                shift
                ;; 
        esac
    done
    set -- "${args[@]}"

    local path="$1"
    local target=

    printDebug $debug "original call type" "$originalCallType" \
                       "path arg" "$path" \
                       "sudo script" "$sudoCommand"

    if [[ -z "$path" ]]; then
        printDebug $debug "path is empty"
        if [[ "$originalCallType" == "$CALL_TYPE_TERMINAL" ]]; then
            printDebug $debug "original call type is terminal, returning PWD"
            printf "%s/\n" "$PWD"
        else
            printDebug $debug "original call type is not terminal"
            if [[ -n "$sudoCommand" ]]; then
                printDebug $debug "sudoCommand is set, returning it"
                printf "%s\n" "$sudoCommand"
            elif [[ -n "${BASH_SOURCE[1]}" ]]; then
                printDebug $debug "BASH_SOURCE[1] is set, returning realpath of it"
                realpath "${BASH_SOURCE[1]}" 2>/dev/null
            else
                printDebug $debug "Falling back to PWD"
                printf "%s/\n" "$PWD" # Fallback
            fi
        fi
        return 0
    fi
    
    if [[ "$path" == "/" ]]; then
        printDebug $debug "path is root"
        printf "/\n"
        return 0
    fi

    if [[ "$path" == /* ]]; then # absolute path
        printDebug $debug "path is absolute"
        target="$(realpath "$path" 2>/dev/null)"
        printDebug $debug "realpath result" "$target"
        if ! printFileOrDirRealPath "$target"; then
            printDebug $debug "printFileOrDirRealPath failed, returning original path"
            printf "%s\n" "$path"
            return 1
        fi
    else # relative path
        printDebug $debug "path is relative"
        local baseDir
        if [[ "$originalCallType" == "$CALL_TYPE_TERMINAL" ]]; then
            printDebug $debug "original call type is terminal, baseDir is PWD"
            baseDir="$PWD"
        else # subprocessed or sourced
            printDebug $debug "original call type is not terminal"
            if [[ -n "$sudoCommand" ]]; then
                printDebug $debug "sudoCommand is set, baseDir is dirname of it"
                baseDir="$(dirname "$sudoCommand")"
            elif [[ -n "${BASH_SOURCE[1]}" ]]; then
                printDebug $debug "BASH_SOURCE[1] is set, baseDir is dirname of realpath of it"
                baseDir="$(dirname "$(realpath "${BASH_SOURCE[1]}" 2>/dev/null)")"
            else
                printDebug $debug "Falling back to PWD for baseDir"
                baseDir="$PWD" # Fallback
            fi
        fi
        
        printDebug $debug "baseDir" "$baseDir" \
                           "combined path" "$baseDir/$path"
        target="$(realpath "$baseDir/$path" 2>/dev/null)"
        printDebug $debug "realpath result" "$target"
        if ! printFileOrDirRealPath "$target"; then
            printDebug $debug "printFileOrDirRealPath failed, returning combined path"
            printf "%s\n" "$baseDir/$path"
            return 1
        fi
    fi
}
export -f theRealPath

if [[ "${FUNCNAME[-1]}" == "main" ]]; then
    theRealPath "$@"
fi