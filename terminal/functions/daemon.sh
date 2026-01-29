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

# Pull automateLinux repo on remote peer by IP
remotePull() {
    local ip="$1"
    if [[ -z "$ip" ]]; then
        echo "Usage: remotePull <ip_address>" >&2
        return 1
    fi
    execOnPeerByIp "$ip" "/opt/automateLinux" "git pull"
}
export -f remotePull

# Build daemon on remote peer by IP
remoteBd() {
    local ip="$1"
    if [[ -z "$ip" ]]; then
        echo "Usage: remoteBd <ip_address>" >&2
        return 1
    fi
    # Source the build script directly since bd is a function
    execOnPeerByIp "$ip" "/opt/automateLinux/daemon" "source ./build.sh"
}
export -f remoteBd

# Pull and build daemon on remote peer by IP
remoteDeployDaemon() {
    local ip="$1"
    if [[ -z "$ip" ]]; then
        echo "Usage: remoteDeployDaemon <ip_address>" >&2
        return 1
    fi

    echo "Pulling changes on $ip..."
    remotePull "$ip" || return 1

    echo "Building daemon on $ip..."
    remoteBd "$ip"
}
export -f remoteDeployDaemon
