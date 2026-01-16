d(){
    if [[ "$1" == "send" || "$1" == "daemon" ]]; then
        daemon "$@"
    else
        daemon send "$@"
    fi
}
export -f d
