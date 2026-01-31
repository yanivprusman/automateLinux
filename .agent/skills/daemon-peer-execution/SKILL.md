---
name: daemon-peer-execution
description: Execute commands on remote peers via daemon networking (replaces SSH)
---

# Daemon Peer Execution Skill

Execute commands on remote peers via the daemon's peer networking infrastructure. Replaces SSH-based execution with daemon-native peer-to-peer communication over WireGuard VPN.

## Problem Solved
**Two critical issues:**

1. **Directory Context**: SSH commands start in the user's home directory, not the target git repository. Running `ssh user@host 'git pull'` fails with "not a git repository" error.

2. **Integration**: SSH is external to the daemon ecosystem. Using daemon peer commands provides:
   - Unified logging in daemon logs
   - No separate SSH key management
   - Integration with peer status tracking
   - Consistent error handling
   - Works with existing peer network topology

## Architecture

### Current Network Topology

```
                    WireGuard VPN (10.0.0.0/24)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│   Worker      │     │    LEADER     │     │   Worker      │
│  10.0.0.4     │────▶│   10.0.0.1    │◀────│  10.0.0.2     │
│  (Laptop)     │     │   (VPS)       │     │  (Desktop)    │
└───────────────┘     └───────────────┘     └───────────────┘
```

- **VPS (10.0.0.1)**: Leader - maintains peer registry in database
- **Desktop (10.0.0.2)**: Worker - queries leader for peer info
- **Laptop (10.0.0.4)**: Worker - queries leader for peer info

### Daemon Commands

#### `execOnPeer` - Execute arbitrary command on remote peer
```bash
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"
```

**What happens:**
1. Local daemon validates peer exists (queries leader if worker)
2. Local daemon connects to target peer via TCP port 3600
3. Sends `execRequest` message to target peer
4. Target daemon executes `cd /opt/automateLinux && git pull 2>&1`
5. Target daemon captures output and exit code
6. Result returned synchronously to caller

#### Convenience Commands

| Command | Description |
|---------|-------------|
| `d remotePull --peer <id>` | Git pull automateLinux on peer |
| `d remoteBd --peer <id>` | Build daemon on peer |
| `d remoteDeployDaemon --peer <id>` | Pull + build daemon (chained) |

**Tab completion**: `d remotePull --peer <TAB>` dynamically queries `d listPeers`.

### Shell Helper Functions

Located in [terminal/functions/daemon.sh](terminal/functions/daemon.sh):

| Function | Usage | Description |
|----------|-------|-------------|
| `getPeerIdByIp` | `getPeerIdByIp 10.0.0.1` | Lookup peer_id from IP address |
| `execOnPeerByIp` | `execOnPeerByIp 10.0.0.1 /path "cmd"` | Execute command on peer by IP |

## Peer Setup

### Initial Configuration

**On leader (VPS - 10.0.0.1):**
```bash
d setPeerConfig --role leader --id vps
```

**On workers (Desktop, Laptop):**
```bash
d registerWorker    # Uses hostname as peer_id, connects to VPS (10.0.0.1)
```

Or with explicit ID: `d setPeerConfig --role worker --id desktop --leader 10.0.0.1`

Workers automatically connect to leader on daemon startup and register themselves.

### Database Sanity Check

Workers should not have peer_registry data (that's leader-only). If a worker was previously a leader or has stale data:
```bash
d dbSanityCheck    # Detects and deletes any peer_registry entries on workers
```

### Verify Peer Status

```bash
d getPeerStatus              # Show my peer config
d listPeers                  # Show all registered peers (queries leader)
```

## How to Invoke

### For Claude Code (AI Assistant)

**CRITICAL RULES:**

1. **Always use daemon commands**, not SSH:
   ```bash
   ✅ d remotePull --peer vps
   ✅ d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git status"
   ✅ d remoteDeployDaemon --peer laptop

   ❌ ssh root@10.0.0.1 'cd /opt/automateLinux && git pull'
   ```

2. **Understand the argument name:**
   - Use `--shellCmd` (not `--command`) to avoid collision with JSON `"command"` key

3. **Check peer status before assuming connectivity:**
   ```bash
   d listPeers                # Verify peer is registered
   d getPeerStatus            # Verify local peer config
   ```

## Common Workflows

### Deploy Changes to All Peers

```bash
# From local machine after committing
git push

# Deploy to each peer
d remoteDeployDaemon --peer vps
d remoteDeployDaemon --peer laptop
```

### Check Status Across Peers

```bash
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git status"
d execOnPeer --peer desktop --directory /opt/automateLinux --shellCmd "git log -1"
```

### Start App on Remote Peer

```bash
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "d startApp --app cad --mode prod"
```

## Error Messages

### "Peer not found: laptop"
Peer hasn't registered with the leader. On the missing peer, run:
```bash
d registerWorker    # Uses hostname, connects to VPS
```

### "Not connected to leader"
Worker daemon hasn't connected to leader. Check:
- Is leader daemon running?
- Is WireGuard VPN up? (`wg show`)
- Can you ping the leader? (`ping 10.0.0.1`)

### "Failed to connect to peer"
Target peer is registered but not reachable. Check:
- Is daemon running on target peer?
- WireGuard connectivity (`ping <peer_ip>`)

### "Timeout waiting for response"
Command took too long (30 second timeout). Check:
- Is the command hanging on target?
- Network latency issues?

## Technical Details

### Command Flow

1. **Local CLI** calls `d execOnPeer ...`
2. **Local daemon** receives via UNIX socket
3. **Peer lookup**: Leader checks database, worker queries leader
4. **TCP connection** to target peer on port 3600 (wg0 interface)
5. **execRequest message** sent as JSON
6. **Target daemon** executes via `popen()`: `cd <dir> && <cmd> 2>&1`
7. **Output capture** streams back to caller
8. **Display** output shown on local machine

### Key Source Files

- [daemon/include/Constants.h](daemon/include/Constants.h) - Command constants
- [daemon/src/cmdPeer.cpp](daemon/src/cmdPeer.cpp) - Handler implementations
- [daemon/src/PeerManager.cpp](daemon/src/PeerManager.cpp) - Peer connection management
- [daemon/src/mainCommand.cpp](daemon/src/mainCommand.cpp) - Command registration
- [terminal/completions/daemon.bash](terminal/completions/daemon.bash) - Tab completions

## Benefits Over SSH

| Feature | SSH Approach | Daemon Approach |
|---------|-------------|-----------------|
| **Authentication** | Requires SSH keys | Uses peer connection |
| **Logging** | Separate logs | Unified daemon logs |
| **Network** | New connection each time | Persistent/on-demand TCP |
| **Directory Context** | Manual `cd &&` | Enforced by command |
| **Tab Completion** | None | Dynamic from peer list |
| **Error Handling** | Shell exit codes | Structured CmdResult |

## Related Documentation

- [CLAUDE.md](CLAUDE.md) - General project guidance, peer networking section
- [peer-commands skill](.agent/skills/peer-commands/SKILL.md) - Quick reference
