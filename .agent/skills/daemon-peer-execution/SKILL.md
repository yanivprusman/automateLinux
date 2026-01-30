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

### Daemon Commands

#### `execOnPeer` - Execute arbitrary command on remote peer
**Desktop initiates:**
```bash
d execOnPeer --peer vps --directory /opt/automateLinux --command "git pull"
```

**What happens:**
1. Desktop daemon validates peer exists and is online
2. Desktop daemon sends `execRequest` message to VPS via TCP port 3600
3. VPS daemon receives request, executes `cd /opt/automateLinux && git pull 2>&1`
4. VPS daemon captures output and exit code
5. VPS daemon returns result to desktop daemon
6. Desktop daemon displays output

**Implementation:**
- [Constants.h:122-123](daemon/include/Constants.h#L122-L123) - Command constants
- [mainCommand.cpp:1882-1951](daemon/src/mainCommand.cpp#L1882-L1951) - Handler functions
- [PeerManager.cpp:203-220](daemon/src/PeerManager.cpp#L203-L220) - Message sending

### Bash Wrapper Functions

Located in [terminal/functions/sshgit.sh](terminal/functions/sshgit.sh)

#### Core Functions

**`execOnPeer <peer_id> <directory> <command>`**
Execute arbitrary command on remote peer.
```bash
execOnPeer vps /opt/automateLinux "git pull"
execOnPeer desktop /home/yaniv/cad-prod "./build.sh"
```

**`gitOnPeer <peer_id> <directory> <git-operation>`**
Convenience wrapper for git operations.
```bash
gitOnPeer vps /opt/automateLinux pull
gitOnPeer vps /opt/automateLinux status
gitOnPeer desktop /opt/automateLinux log --oneline -5
```

**`getCurrentPeerId()`**
Detects current machine based on WireGuard IP (10.0.0.x).
- 10.0.0.1 → vps
- 10.0.0.2 → desktop
- 10.0.0.4 → laptop

#### Convenience Functions

| Function | Description | Equivalent Command |
|----------|-------------|-------------------|
| `vpsPull` | Pull automateLinux on VPS | `gitOnPeer vps /opt/automateLinux pull` |
| `vpsCADPull` | Pull CAD repo on VPS | `gitOnPeer vps /home/yaniv/cad-prod pull` |
| `vpsInstall` | Run install.sh on VPS (with --minimal) | `execOnPeer vps /opt/automateLinux "./install.sh --minimal"` |
| `vpsExec <cmd>` | Execute command on VPS in automateLinux dir | `execOnPeer vps /opt/automateLinux "<cmd>"` |
| `desktopExec <cmd>` | Execute command on desktop in automateLinux dir | `execOnPeer desktop /opt/automateLinux "<cmd>"` |

#### Legacy SSH Functions

`sshGit` and `sshCmd` are retained for backward compatibility but deprecated. They print a notice to prefer daemon-based execution.

## When to Use This Skill

**Use daemon peer execution whenever you need to:**
- Run git commands on remote peers (pull, status, log, diff)
- Execute build scripts on remote machines
- Run installation scripts on VPS
- Check status or logs on remote peers
- Any operation requiring directory context on remote machines

**Requirements:**
- Peers must be configured (see Peer Setup below)
- Target peer must be online and connected
- WireGuard VPN must be active (wg0 interface up)

## Peer Setup

### Initial Configuration

**On leader (Desktop - 10.0.0.2):**
```bash
d setPeerConfig --role leader --id desktop
```

**On workers (VPS - 10.0.0.1, Laptop - 10.0.0.4):**
```bash
d setPeerConfig --role worker --id vps --leader 10.0.0.2
d setPeerConfig --role worker --id laptop --leader 10.0.0.2
```

Workers automatically connect to leader on daemon startup.

### Verify Peer Status

```bash
d getPeerStatus              # Show my peer config
d listPeers                  # Show all registered peers
```

## How to Invoke

### For Claude Code (AI Assistant)

**CRITICAL RULES:**

1. **Always use daemon-based functions**, not SSH:
   ```bash
   ✅ vpsPull
   ✅ gitOnPeer vps /opt/automateLinux pull
   ✅ execOnPeer vps /opt/automateLinux "./install.sh --minimal"

   ❌ ssh root@10.0.0.1 'cd /opt/automateLinux && git pull'
   ❌ sshCmd root@10.0.0.1 /opt/automateLinux git pull
   ```

2. **Understand execution context:**
   - Functions run **locally** but execute **remotely**
   - `vpsPull` runs on current machine, executes git pull on VPS
   - After `vpsPull`, you still need to source bashrc ON VPS if you changed bash functions
   - To source on VPS: `execOnPeer vps /opt/automateLinux "source ~/.bashrc"`

3. **Don't assume functions are available remotely after local sourcing:**
   ```bash
   ✅ source terminal/functions/sshgit.sh    # Functions available LOCALLY
   ✅ vpsPull                                # Executes git pull ON VPS
   ❌ vpsPull                                # Won't work ON VPS unless bashrc sourced there

   # To make functions available on VPS:
   ✅ vpsPull                                # Pull the updated sshgit.sh to VPS
   ✅ execOnPeer vps ~ "source ~/.bashrc"   # Source bashrc on VPS
   ```

4. **Machine detection:**
   - Use `getCurrentPeerId` to detect which machine you're on
   - Functions prevent executing on self (use local commands instead)
   - If `getCurrentPeerId` returns "laptop", don't try `execOnPeer laptop ...`

### For Human Users

Functions are automatically available after running `user_install.sh` (sourced in bashrc).

## Common Workflows

### Update VPS AutomateLinux Installation

```bash
# 1. Push changes from current machine (desktop/laptop)
git add terminal/functions/sshgit.sh
git commit -m "Update sshgit functions"
git push

# 2. Pull on VPS
vpsPull

# 3. Reinstall (if needed)
vpsInstall

# 4. Source bashrc on VPS (if bash functions changed)
execOnPeer vps ~ "source ~/.bashrc"
```

### Update VPS CAD Deployment

```bash
# From CAD repo, after committing changes
git push

# Pull and rebuild on VPS
vpsCADPull
execOnPeer vps /home/yaniv/cad-prod "./build.sh"
```

### Check Status on Multiple Peers

```bash
gitOnPeer vps /opt/automateLinux status
gitOnPeer desktop /opt/automateLinux status
d listPeers  # See which peers are online
```

### Debug Remote Build Issues

```bash
# Check what's in the directory
execOnPeer vps /opt/automateLinux "ls -la"

# Check git status
gitOnPeer vps /opt/automateLinux status

# View recent logs
execOnPeer vps /opt/automateLinux "tail -n 50 /var/log/daemon.log"
```

## Error Messages

### "Peer not found: vps"
Peer hasn't been registered. Check with `d listPeers`. May need to restart daemon on that peer.

### "Peer is offline: vps"
Peer is registered but not connected. Check:
- Is daemon running on target peer?
- Is WireGuard VPN up? (`wg show`)
- Can you ping the peer IP? (`ping 10.0.0.1`)

### "Failed to send command to peer"
Network issue or peer disconnected. Check peer connection with `d listPeers`.

### "Cannot exec on self (laptop)"
You tried to use `execOnPeer laptop ...` while on the laptop. Run the command locally instead.

## Execution Model Clarification

This is the single most important concept to understand:

```
┌──────────────────────────────────────────────────────────┐
│  LAPTOP (10.0.0.4)                                        │
│  ├─ You are here                                          │
│  ├─ You source sshgit.sh → functions available LOCALLY   │
│  ├─ You run: vpsPull                                      │
│  │   ├─ Function runs on LAPTOP                           │
│  │   ├─ Daemon sends command to VPS                       │
│  │   └─ Result displayed on LAPTOP                        │
│  └─ Functions like vpsPull are NOT available on VPS yet   │
└──────────────────────────────────────────────────────────┘
                           │
                           │ daemon peer network (TCP 3600)
                           ▼
┌──────────────────────────────────────────────────────────┐
│  VPS (10.0.0.1)                                           │
│  ├─ Daemon receives execRequest                           │
│  ├─ Executes: cd /opt/automateLinux && git pull          │
│  ├─ Captures output                                       │
│  ├─ Functions like vpsPull NOT available here             │
│  └─ To make them available:                               │
│      1. vpsPull (pulls updated sshgit.sh)                 │
│      2. execOnPeer vps ~ "source ~/.bashrc"               │
└──────────────────────────────────────────────────────────┘
```

**Key insight:** Sourcing locally does NOT make functions available remotely. Remote execution is through daemon message passing, not bash function availability.

## Technical Details

### Command Flow

1. **Local bash function** (`vpsPull`) calls `d execOnPeer ...`
2. **Local daemon** (`/run/automatelinux/automatelinux-daemon.sock`) receives command
3. **Validation** checks peer exists and is online
4. **JSON message** sent via TCP socket (port 3600) over WireGuard to target peer
5. **Remote daemon** receives `execRequest` message
6. **Execution** via `popen()`: `cd <directory> && <command> 2>&1`
7. **Output capture** reads stdout/stderr
8. **Result return** sent back via TCP to requesting peer
9. **Display** output shown to user on local machine

### Files Modified

- [daemon/include/Constants.h](daemon/include/Constants.h) - Added command constants
- [daemon/src/mainCommand.cpp](daemon/src/mainCommand.cpp) - Added handlers and registration
- [terminal/functions/sshgit.sh](terminal/functions/sshgit.sh) - Complete rewrite for daemon-based execution
- [SKILL.md](SKILL.md) - This documentation

### Backward Compatibility

Legacy SSH functions (`sshGit`, `sshCmd`) remain available for:
- Situations where daemon peer connection is unavailable
- Connecting to machines not in the peer network
- Emergency access when daemon is down

These functions now display a deprecation notice recommending daemon-based alternatives.

## Testing

```bash
# 1. Verify peer connectivity
d listPeers

# 2. Test simple command
execOnPeer vps /opt/automateLinux "pwd"

# 3. Test git operation
gitOnPeer vps /opt/automateLinux status

# 4. Test convenience function
vpsPull

# 5. Verify output is displayed locally
```

## Benefits Over SSH

| Feature | SSH Approach | Daemon Approach |
|---------|-------------|-----------------|
| **Authentication** | Requires SSH keys | Uses existing peer connection |
| **Logging** | SSH logs + command output | Unified in daemon logs |
| **Network** | Separate SSH connection | Reuses persistent peer TCP socket |
| **Integration** | External tool | Native daemon feature |
| **Error Handling** | Shell exit codes | Structured CmdResult |
| **Peer Awareness** | Must specify IP/user | Uses peer ID from network |
| **Directory Context** | Manual `cd &&` | Enforced by daemon command |

## Related Documentation

- [CLAUDE.md](CLAUDE.md) - General project guidance, peer networking section
- [daemon/src/PeerManager.cpp](daemon/src/PeerManager.cpp) - Peer connection management
- [daemon/src/mainCommand.cpp](daemon/src/mainCommand.cpp) - Command handlers
- WireGuard VPN configuration - Network topology (10.0.0.x)
