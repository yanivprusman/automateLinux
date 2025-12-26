evsievep() {
    local path=()
    local print=()
    local used_dev=0

    local orig_arg_count=$#
    while [[ $# -gt 0 ]]; do
        case $1 in
            -k)
                path+=( $(d send getKeyboardPath) )
                used_dev=1
                shift
                ;;
            -m)
                path+=( $(d send getMousePath) )
                used_dev=1
                shift
                ;;
            -p)
                [[ -n $2 ]] || { echo "Error: -p requires an argument" >&2; return 1; }
                print+=( "$2" )
                shift 2
                ;;
            *)
                shift
                ;;
        esac
    done
    if [[ $orig_arg_count -eq 0 ]]; then
        path=( /dev/input/event* )
    elif (( ${#path[@]} == 0 )); then
        path+=( $(d send getKeyboardPath) )
    fi
    if (( used_dev )); then
        if [[ -e /dev/input/by-id/corsairKeyBoardLogiMouse ]]; then
            path+=( /dev/input/by-id/corsairKeyBoardLogiMouse )
        fi
    fi
    
    local input_args=()
    for p in "${path[@]}"; do
        input_args+=( --input "$p" )
    done

    echo "sudo evsieve ${input_args[*]} --print ${print[*]} format=direct"
    sudo evsieve "${input_args[@]}" --print "${print[@]}" format=direct
}
export -f evsievep

restartEvsieveOnSave(){
    local file=$(daemon send getFile --fileName corsairKeyBoardLogiMouseAll.sh)
    local dir=$(dirname "$file")
    local base=$(basename "$file")
    echo "monitoring $file"
    inotifywait -m -e close_write "$dir" | while read -r _ _ f; do
        if [[ "$f" == "$base" ]]; then
            # Small debounce to avoid race conditions during rapid saves
            sleep 0.2
            daemon send setKeyboard --enable true > /dev/null 2>&1
        fi
    done
}
export -f restartEvsieveOnSave

debugEvsieve(){
    # combine the output of evsieveOutput.log and evsieveErr.log to combined.log
    log evsieve $(d getFile fileName=combined.log)
    # update changes of evsieve imedietly on save in vscode
    restartVSCodeKeyEvsieveOnSave &
}
export -f debugEvsieve