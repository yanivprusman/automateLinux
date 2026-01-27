grantUserAccess() {
    local user=$1
    local target="/home/yaniv"
    sudo setfacl -R -m u:$user:rwX "$target"
    sudo setfacl -R -d -m u:$user:rwX "$target"
}

# Run a bash function as sudo
# Usage: run_as_sudo functionName [args...]
run_as_sudo() {
    local func_name="$1"
    shift
    sudo bash -c "$(declare -f "$func_name"); $(printf '%q ' "$func_name" "$@")"
}