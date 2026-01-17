daemon() {
    if [[ "$1" == "send" || "$1" == "daemon" || "$1" == "--help" ]]; then
        command daemon "$@"
    else
        command daemon send "$@"
    fi
}
export -f daemon

d() {
    daemon "$@"
}
export -f d
