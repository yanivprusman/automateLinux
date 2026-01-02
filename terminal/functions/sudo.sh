grantUserAccess() {
    local user=$1
    local target="/home/yaniv"
    sudo setfacl -R -m u:$user:rwX "$target"
    sudo setfacl -R -d -m u:$user:rwX "$target"
}