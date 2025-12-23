
log() {
    local what="" target="" reset=false args=()
    for arg in "$@"; do
        case "$arg" in
            -reset) reset=true ;;
            evsieve|anotherCommand) what="$arg" ;;
            *) args+=("$arg") ;;
        esac
    done
    target="${args[0]}"
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
            tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveErr.log" >>"$target" &
            tail -f "${AUTOMATE_LINUX_DATA_DIR}evsieveOutput.log" >>"$target" &
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

restartVSCodeKeyEvsieveOnSave(){
    local file=$(d getFile fileName=corsairKeyBoardLogiMouseCode.sh)
    echo monitoring $file
    inotifywait -m -e close_write "$file" | while read _; do
        daemon setKeyboard > /dev/null 2>&1
        daemon setKeyboard > /dev/null 2>&1
    done
}
export -f restartVSCodeKeyEvsieveOnSave

debugEvsieve(){
    # combine the output of evsieveOutput.log and evsieveErr.log to combined.log
    log evsieve $(d getFile fileName=combined.log)
    # update changes of evsieve imedietly on save in vscode
    restartVSCodeKeyEvsieveOnSave &
}
export -f debugEvsieve