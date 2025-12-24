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

tailEvsieve(){
    tail -f /home/yaniv/coding/automateLinux/data/evsieveOutput.log | awk '{ print strftime("[%Y-%m-%d %H:%M:%S]"), $0 }'
}
export -f tailEvsieve
