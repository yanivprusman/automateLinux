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

# Get peer_id from IP address
getPeerIdByIp() {
    local ip="$1"
    if [[ -z "$ip" ]]; then
        echo "Usage: getPeerIdByIp <ip_address>" >&2
        return 1
    fi
    d listPeers | jq -r --arg ip "$ip" '.[] | select(.ip_address == $ip) | .peer_id'
}
export -f getPeerIdByIp

# Execute command on remote peer by IP
execOnPeerByIp() {
    local ip="$1"
    local dir="$2"
    local cmd="$3"

    if [[ -z "$ip" || -z "$dir" || -z "$cmd" ]]; then
        echo "Usage: execOnPeerByIp <ip_address> <directory> <command>" >&2
        return 1
    fi

    local peer_id
    peer_id=$(getPeerIdByIp "$ip")

    if [[ -z "$peer_id" ]]; then
        echo "No peer found with IP: $ip" >&2
        return 1
    fi

    d execOnPeer --peer "$peer_id" --directory "$dir" --shellCmd "$cmd"
}
export -f execOnPeerByIp

# Create a test commit and push to verify deployment pipeline
bumpBuild() {
    local dir="/opt/automateLinux"
    local file="$dir/.deploy-test"
    date +%s > "$file"
    git -C "$dir" add "$file"
    git -C "$dir" commit -m "Test deploy bump"
    git -C "$dir" push
    echo "Pushed test commit. Current version: $(git -C "$dir" rev-list --count HEAD)"
}
export -f bumpBuild
