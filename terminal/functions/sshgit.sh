# Daemon Peer Execution Functions
# Functions for running commands on remote peers via daemon peer networking

# Detect current machine based on WireGuard IP
getCurrentPeerId() {
    local wg_ip=$(ip addr show wg0 2>/dev/null | grep -oP 'inet \K[\d.]+')
    case "$wg_ip" in
        10.0.0.1) echo "vps" ;;
        10.0.0.2) echo "desktop" ;;
        10.0.0.4) echo "laptop" ;;
        *) echo "unknown" ;;
    esac
}

# Execute command on remote peer via daemon
# Usage: execOnPeer <peer_id> <directory> <command>
# Example: execOnPeer vps /opt/automateLinux "git pull"
execOnPeer() {
    local peer_id="$1"
    local directory="$2"
    shift 2
    local command="$@"

    if [ -z "$peer_id" ] || [ -z "$directory" ] || [ -z "$command" ]; then
        echo "Usage: execOnPeer <peer_id> <directory> <command>"
        echo "Example: execOnPeer vps /opt/automateLinux 'git pull'"
        echo ""
        echo "Available peers: vps, desktop, laptop"
        return 1
    fi

    # Don't try to exec on self
    local current_peer=$(getCurrentPeerId)
    if [ "$peer_id" = "$current_peer" ]; then
        echo "Error: Cannot exec on self ($current_peer). Run command locally instead."
        return 1
    fi

    d execOnPeer --peer "$peer_id" --directory "$directory" --command "$command"
}

# Git-specific wrapper for peer execution
# Usage: gitOnPeer <peer_id> <directory> <git-operation>
# Example: gitOnPeer vps /opt/automateLinux pull
gitOnPeer() {
    local peer_id="$1"
    local directory="$2"
    shift 2
    local git_operation="$@"

    if [ -z "$peer_id" ] || [ -z "$directory" ] || [ -z "$git_operation" ]; then
        echo "Usage: gitOnPeer <peer_id> <directory> <git-operation>"
        echo "Example: gitOnPeer vps /opt/automateLinux pull"
        return 1
    fi

    execOnPeer "$peer_id" "$directory" "git $git_operation"
}

# Convenience functions for common operations

# Pull automateLinux on VPS
vpsPull() {
    gitOnPeer vps /opt/automateLinux pull
}

# Pull CAD repo on VPS
vpsCADPull() {
    gitOnPeer vps /home/yaniv/cad-prod pull
}

# Install automateLinux on VPS (with --minimal flag for headless)
vpsInstall() {
    execOnPeer vps /opt/automateLinux "./install.sh --minimal"
}

# Execute command on VPS in automateLinux directory
vpsExec() {
    local command="$@"
    if [ -z "$command" ]; then
        echo "Usage: vpsExec <command>"
        echo "Example: vpsExec './install.sh --minimal'"
        return 1
    fi
    execOnPeer vps /opt/automateLinux "$command"
}

# Execute command on desktop in automateLinux directory
desktopExec() {
    local command="$@"
    if [ -z "$command" ]; then
        echo "Usage: desktopExec <command>"
        echo "Example: desktopExec 'git status'"
        return 1
    fi
    execOnPeer desktop /opt/automateLinux "$command"
}

# Legacy SSH fallback functions (use only if daemon peer connection unavailable)
# These are kept for backward compatibility but daemon commands are preferred

sshGit() {
    local user_host="$1"
    local directory="$2"
    shift 2
    local git_command="$@"

    if [ -z "$user_host" ] || [ -z "$directory" ] || [ -z "$git_command" ]; then
        echo "Usage: sshGit <user@host> <directory> <git-command>"
        echo "Example: sshGit root@10.0.0.1 /opt/automateLinux pull"
        echo ""
        echo "Note: Prefer using gitOnPeer instead for daemon-based execution"
        return 1
    fi

    ssh "$user_host" "cd $directory && git $git_command"
}

sshCmd() {
    local user_host="$1"
    local directory="$2"
    shift 2
    local command="$@"

    if [ -z "$user_host" ] || [ -z "$directory" ] || [ -z "$command" ]; then
        echo "Usage: sshCmd <user@host> <directory> <command>"
        echo "Example: sshCmd root@10.0.0.1 /opt/automateLinux ./install.sh"
        echo ""
        echo "Note: Prefer using execOnPeer instead for daemon-based execution"
        return 1
    fi

    ssh "$user_host" "cd $directory && $command"
}
