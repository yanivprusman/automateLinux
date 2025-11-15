theRealPath() {
    local botomMostElement="${FUNCNAME[-1]}" 
    if [[ "$botomMostElement" == "main" ]]; then
        # subprocessed
        realpath ${BASH_SOURCE[1]}
    elif [[ "$botomMostElement" == "source" ]]; then
        # sourced
        realpath ${BASH_SOURCE[1]}
    else
        # called from terminal
        if [[ $1 == /* ]]; then
            realpath "$1"
        else
            realpath "${PWD}/$1"
        fi
    fi
}
export -f theRealPath

printTheRealPath() {
    local botomMostElement="${FUNCNAME[-1]}" 
    if [[ "$botomMostElement" == "main" ]]; then
        echo "subprocessed"
    elif [[ "$botomMostElement" == "source" ]]; then
        echo "sourced"
    else
        echo "called from terminal"
    fi
    echo -e "${GREEN}FUNCNAME array:${NC}"
    for i in "${!FUNCNAME[@]}"; do
        printf "\t%d: %s\n" "$i" "${FUNCNAME[i]}"
    done
    echo -e "${GREEN}BASH_SOURCE array:${NC}"
    for i in "${!BASH_SOURCE[@]}"; do
        printf "\t%d: %s\n" "$i" "${BASH_SOURCE[i]}"
    done
    echo -e "${GREEN}BASH_LINENO array:${NC}"
    for i in "${!BASH_LINENO[@]}"; do
        printf "\t%d: %s\n" "$i" "${BASH_LINENO[i]}"
    done
}
export -f printTheRealPath

sudoTheRealPath() {
:
}
export -f sudoTheRealPath

# FUNCNAME
#         An  array  variable containing the names of all shell functions currently in the execution call stack.  The element with index 0
#         is the name of any currently-executing shell function.  The bottom-most element (the one with  the  highest  index)  is  "main".
#         This variable exists only when a shell function is executing.  Assignments to FUNCNAME have no effect.  If FUNCNAME is unset, it
#         loses its special properties, even if it is subsequently reset.
#         This  variable can be used with BASH_LINENO and BASH_SOURCE.  Each element of FUNCNAME has corresponding elements in BASH_LINENO
#         and BASH_SOURCE to describe the call stack.  For instance, ${FUNCNAME[$i]} was called from the file ${BASH_SOURCE[$i+1]} at line
#         number ${BASH_LINENO[$i]}.  The caller builtin displays the current call stack using this information.
