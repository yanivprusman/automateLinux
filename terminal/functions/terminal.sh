# Terminal and shell utilities

resetPromptColor() {
    echo -en "\033[00m"
}
export -f resetPromptColor

cd() {
    if [ "$1" = "..." ]; then
        builtin cd ../..
    else
        builtin cd "$@"
    fi
}
export -f cd

outputToSelf(){
    exec 1>/dev/pts/$(tty | sed 's:/dev/pts/::')
}
export -f outputToSelf

showTime(){
    date +%H:%M
}
export -f showTime

echoBlockSeparator() {
    echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
}
export -f echoBlockSeparator

getCurrentShell(){
    readlink /proc/$$/exe
    # or echo $0 ?
}
export -f getCurrentShell

changeTitle() {
    local prepend="" append="" replace="" reset=""
    while [ "$#" -gt 0 ]; do
        case "$1" in
            -prepend) prepend="$2"; shift 2;;
            -append)  append="$2"; shift 2;;
            -replace) replace="$2"; shift 2;;
            -reset) reset="1"; shift 1;;
            *) return 1;;
        esac
    done
    [ -n "$reset" ] && PS1="$AUTOMATE_LINUX_PS1" && return 0
    local current_title
    current_title=$(printf "%s" "$PS1" | sed -n 's/.*\\\[\\e]0;\(.*\)\\a.*/\1/p')
    [ -z "$current_title" ] && return 0
    local new_title="$current_title"
    [ -n "$prepend" ] && new_title="${prepend}${new_title}"
    [ -n "$append" ]  && new_title="${new_title}${append}"
    [ -n "$replace" ] && new_title="${replace}"
    PS1=$(printf "%s" "$PS1" |
    sed 's#\\\[\\e]0;.*\\a#\\[\\e]0;'"$(printf "%s" "$new_title" | sed 's/[\&]/\\&/g')"'\\a#')
}
export -f changeTitle

# log() {
#     local what="" target="" reset=false
#     while [ "$#" -gt 0 ]; do
#         case "$1" in
#             -reset) reset=true; shift ;;
#             evsieve|anotherCommand)
#                 what="$1"; shift ;;
#             *)
#                 target="$1"; shift ;;
#         esac
#     done
#     if [ -z "$what" ]; then
#         echo "Usage: log <command> <target>"
#         return 1
#     fi
#     if [ -z "$target" ]; then
#         echo "Add target: log $what <target>"
#         return 1
#     fi
#     case "$what" in
#         evsieve)
#             if [ "$reset" = true ]; then
#                 > "${AUTOMATE_LINUX_DATA_DIR}evsieveErr.log"
#                 > "${AUTOMATE_LINUX_DATA_DIR}evsieveOutput.log"
#             fi
#             tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveErr.log" >"$target" &
#             tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveOutput.log" >"$target" &
#             ;;
#         anotherCommand)
#             # future commands
#             ;;
#         *)
#             echo "Unregistered log $what "
#             return 1
#             ;;
#     esac
#     echo -e "${GREEN} Logging"
# }
log() {
    local what="" target="" reset=false args=()
    # First pass: separate flags and commands
    for arg in "$@"; do
        case "$arg" in
            -reset) reset=true ;;
            evsieve|anotherCommand) what="$arg" ;;
            *) args+=("$arg") ;;
        esac
    done

    target="${args[0]}"  # assign the first remaining argument as target

    if [ -z "$what" ]; then
        echo "Usage: log <command> <target>"
        return 1
    fi
    if [ -z "$target" ]; then
        echo "Add target: log $what <target>"
        return 1
    fi

    case "$what" in
        evsieve)
            if [ "$reset" = true ]; then
                sudo sh -c "> '${AUTOMATE_LINUX_DATA_DIR}evsieveErr.log'"
                sudo sh -c "> '${AUTOMATE_LINUX_DATA_DIR}evsieveOutput.log'"
            fi
            tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveErr.log" >"$target" &
            tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveOutput.log" >"$target" &
            ;;
        anotherCommand)
            # future commands
            ;;
        *)
            echo "Unregistered log $what "
            return 1
            ;;
    esac
    echo -e "${GREEN} Logging"
}
export -f log
