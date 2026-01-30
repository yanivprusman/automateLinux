---
name: peer-commands
description: Execute commands on remote peers via daemon, never use direct SSH
---

# Peer Command Execution

## Primary Commands (ALWAYS USE THESE)

```bash
# Execute arbitrary command on peer
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"

# Convenience commands (native daemon commands)
d remotePull --peer vps            # git pull automateLinux
d remoteBd --peer vps              # build daemon
d remoteDeployDaemon --peer vps    # pull + build daemon
```

**Tab completion available**: `d remotePull --peer <TAB>` shows registered peers.

## Peer Network

| Peer | IP | Role |
|------|-----|------|
| VPS | 10.0.0.1 | Leader |
| Desktop | 10.0.0.2 | Worker |
| Laptop | 10.0.0.4 | Worker |

Workers must connect to the leader. The leader maintains the peer registry. Workers query the leader to resolve peer IDs.

## Key Commands Reference

| Command | Description |
|---------|-------------|
| `d registerWorker` | Register as worker (uses hostname, connects to VPS) |
| `d listPeers` | List all registered peers (queries leader) |
| `d getPeerStatus` | Show local peer config |
| `d dbSanityCheck` | Check/fix worker database (delete leader-only data) |
| `d execOnPeer --peer <id> --directory <path> --shellCmd <cmd>` | Execute shell command |
| `d remotePull --peer <id>` | Git pull automateLinux on peer |
| `d remoteBd --peer <id>` | Build daemon on peer |
| `d remoteDeployDaemon --peer <id>` | Pull + build daemon |

## Shell Helper Functions

For ad-hoc IP-based commands when peer ID is unknown:

```bash
getPeerIdByIp 10.0.0.1                              # Returns: vps
execOnPeerByIp 10.0.0.1 /opt/automateLinux "pwd"    # Execute by IP
```

## Bootstrap Exception (peer has old daemon)

When a peer needs the new daemon code before `execOnPeer` works, use SSH wrapper:

```bash
source /opt/automateLinux/terminal/functions/sshgit.sh
sshCmd root@10.0.0.1 /opt/automateLinux "git pull && cd daemon && source ./build.sh"
```

**NEVER use raw SSH:**
```bash
# WRONG - will fail with "not a git repository"
ssh 10.0.0.1 "git pull"
```

## Registering a New Peer

On the new peer:
```bash
d registerWorker    # Uses hostname as peer_id, connects to VPS (10.0.0.1)
```

Or with explicit ID: `d setPeerConfig --role worker --id <peer_name> --leader 10.0.0.1`

The peer will automatically connect to the leader and register itself.

## Troubleshooting

- **"Peer not found"**: Run `d listPeers` - peer must register with leader first
- **"Failed to connect"**: Check `wg show`, `ping <ip>`, `systemctl status daemon.service`
- **"Not connected to leader"**: Worker daemon needs to connect first, restart daemon or check config
