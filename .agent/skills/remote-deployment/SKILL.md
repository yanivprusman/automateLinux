---
name: remote-deployment
description: Deploy code changes to remote peers over WireGuard VPN
---

# Remote Deployment

Execute commands on remote peers connected via WireGuard VPN. The leader daemon establishes on-demand TCP connections to peers and sends shell commands for execution.

## Shell Functions

Defined in `terminal/functions/daemon.sh`:

### `remoteDeployDaemon <ip>`

Deploy automateLinux daemon changes to a remote peer. Runs git pull followed by daemon build.

```bash
remoteDeployDaemon 10.0.0.1
# Output:
# Pulling changes on 10.0.0.1...
# Exec request sent to vps
# Building daemon on 10.0.0.1...
# Exec request sent to vps
```

### `remotePull <ip>`

Pull latest git changes on a remote peer.

```bash
remotePull 10.0.0.1
```

### `remoteBd <ip>`

Build and restart daemon on a remote peer.

```bash
remoteBd 10.0.0.1
```

### `execOnPeerByIp <ip> <directory> <command>`

Execute arbitrary shell command on a remote peer.

```bash
execOnPeerByIp 10.0.0.1 /opt/automateLinux "git status"
execOnPeerByIp 10.0.0.1 /home/user "ls -la"
```

### `getPeerIdByIp <ip>`

Lookup peer_id from IP address using the peer registry.

```bash
getPeerIdByIp 10.0.0.1
# Output: vps
```

## Low-Level Daemon Command

```bash
d execOnPeer --peer <peer_id> --directory <path> --shellCmd <command>
```

**Important:** Use `--shellCmd` not `--command` (the latter conflicts with the JSON protocol's command key).

## Architecture

```
Desktop (Leader)                      VPS (Worker)
     │                                     │
     │  1. execOnPeer --peer vps           │
     │     --shellCmd "git pull"           │
     ├────────────────────────────────────>│
     │        TCP:3600 over wg0            │
     │                                     │
     │  2. Daemon receives execRequest     │
     │     and runs: popen("git pull")     │
     │                                     │
```

## On-Demand Connections

The leader establishes TCP connections to peers automatically when needed:

1. `execOnPeer` checks if peer is connected in memory
2. If not, looks up peer IP in MySQL `peer_registry` table
3. Connects to peer's daemon on port 3600 (wg0 interface)
4. Sends the `execRequest` JSON message
5. Connection is cached for subsequent commands

## Bootstrap Requirement

Remote peers must have the `handleExecRequest` code before they can process commands. For a new peer or after major daemon changes:

```bash
# SSH to peer manually first
ssh user@10.0.0.1
cd /opt/automateLinux && git pull
cd daemon && source ./build.sh
exit

# Now remote deployment works
remoteDeployDaemon 10.0.0.1
```

## Environment Variables

Uses `$kamateraIp` (10.0.0.1) for VPS convenience:

```bash
remoteDeployDaemon $kamateraIp
```

## Troubleshooting

### "Peer not found"

The peer isn't in the database. Check with:
```bash
d listPeers
```

If missing, the peer needs to connect to the leader first:
```bash
# On the remote peer:
d setPeerConfig --role worker --id vps --leader 10.0.0.2
```

### "Failed to connect to peer"

- Verify WireGuard is running: `wg show`
- Check peer is reachable: `ping 10.0.0.1`
- Verify daemon is running on peer: `systemctl status daemon.service`
- Check peer is listening on port 3600: `ss -tlnp | grep 3600`

### Command sent but nothing happens

The remote daemon may not have the `execRequest` handler. Bootstrap manually via SSH.

## CRITICAL: Always Specify Directory

When executing commands on remote peers, you MUST always specify the working directory. The daemon and SSH do NOT automatically use `/opt/automateLinux`.

### Using execOnPeer
```bash
# CORRECT - directory is explicit:
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"

# The --directory parameter is REQUIRED
```

### Using SSH
```bash
# CORRECT - cd to directory first:
ssh 10.0.0.1 "cd /opt/automateLinux && git pull"
ssh 10.0.0.1 "cd /opt/automateLinux && git log --oneline -5"

# WRONG - will fail with "not a git repository":
ssh 10.0.0.1 "git pull"
ssh 10.0.0.1 "git log"
```

### Why This Matters
- SSH starts in the user's home directory, not the project directory
- `execOnPeer` runs commands in the specified `--directory`
- Forgetting the directory causes "not a git repository" errors
- Always include `cd /opt/automateLinux &&` when using SSH for git commands

### Verification
After running remote commands, verify success by checking git state:
```bash
# Via SSH (include directory!):
ssh 10.0.0.1 "cd /opt/automateLinux && git log --oneline -3"

# Or use the shell function:
execOnPeerByIp 10.0.0.1 /opt/automateLinux "git log --oneline -3"
```
