# SSH Git Operations
# Functions for running git commands on remote servers

# Run git command on remote server in specified directory
# Usage: sshGit <user@host> <directory> <git-command>
# Example: sshGit root@10.0.0.1 /opt/automateLinux pull
sshGit() {
    local user_host="$1"
    local directory="$2"
    shift 2
    local git_command="$@"

    if [ -z "$user_host" ] || [ -z "$directory" ] || [ -z "$git_command" ]; then
        echo "Usage: sshGit <user@host> <directory> <git-command>"
        echo "Example: sshGit root@10.0.0.1 /opt/automateLinux pull"
        return 1
    fi

    ssh "$user_host" "cd $directory && git $git_command"
}

# Convenience function for pulling on VPS automateLinux
vpsPull() {
    sshGit root@10.0.0.1 /opt/automateLinux pull
}

# Convenience function for pulling CAD repo on VPS
vpsCADPull() {
    sshGit yaniv@10.0.0.1 ~/cad-prod pull
}

# Run any command on remote server in specified directory
# Usage: sshCmd <user@host> <directory> <command>
# Example: sshCmd root@10.0.0.1 /opt/automateLinux ./install.sh
sshCmd() {
    local user_host="$1"
    local directory="$2"
    shift 2
    local command="$@"

    if [ -z "$user_host" ] || [ -z "$directory" ] || [ -z "$command" ]; then
        echo "Usage: sshCmd <user@host> <directory> <command>"
        echo "Example: sshCmd root@10.0.0.1 /opt/automateLinux ./install.sh"
        return 1
    fi

    ssh "$user_host" "cd $directory && $command"
}
