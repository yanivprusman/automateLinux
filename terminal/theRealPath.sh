theRealPath() {
    local dir
    dir="$(dirname "${BASH_SOURCE[0]}")"
    realpath "$dir/$1"
}
export -f theRealPath
sudoTheRealPath() {
    local target
    target="$(theRealPath "$1")"
    echo "sudoTheRealPath running: $target"
    sudo bash "$target" "${@:2}"   # pass additional args
}
export -f sudoTheRealPath

