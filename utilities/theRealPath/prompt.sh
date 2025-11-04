sourceb 
myBash=/home/yaniv/coding/automateLinux/bashrc/.bashMyBashrcTheRealPath 
cls 
tee "$myBash" <<'EOF' | sed $'s/^/\t/'
theRealPath() {
    local script_path target execute=false run_as_sudo=false

    # Parse optional flags
    while [[ "$1" == --* ]]; do
        case "$1" in
            --exec) execute=true ;;
            --sudo) run_as_sudo=true ;;
            *) echo "Unknown option: $1"; return 1 ;;
        esac
        shift
    done

    if [[ -z "$1" ]]; then
        echo "Usage: theRealPath [--exec] [--sudo] <script> [args...]"
        return 1
    fi

    # Resolve the target path
    # If $1 is absolute, realpath works directly
    # If $1 is relative, resolve relative to current working directory
    target="$(realpath "$1")"
    shift  # move past script name

    if [[ "$execute" == true ]]; then
        if [[ "$run_as_sudo" == true ]]; then
            echo "Running with sudo: $target $*"
            sudo bash "$target" "$@"
        else
            echo "Running: $target $*"
            bash "$target" "$@"
        fi
    else
        # Just return the resolved path
        echo "$target"
    fi
}
export -f theRealPath
EOF
alias 1restart 
which restartCorsairKeyBoardLogiMouseService.sh 
cat /home/yaniv/coding/automateLinux/symlinks/restartCorsairKeyBoardLogiMouseService.sh 
restartCorsairKeyBoardLogiMouseService.sh

