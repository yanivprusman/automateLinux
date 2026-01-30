---
name: peer-commands
description: Execute commands on remote peers via daemon, never use direct SSH
---

# Peer Command Execution

**DO NOT USE SSH.** Execute commands on remote peers via the daemon's peer networking.

## Primary Method

```bash
# By peer ID
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"

# By IP (uses helper function)
execOnPeerByIp 10.0.0.1 /opt/automateLinux "git status"
```

**Note:** Argument is `--shellCmd` (not `--command`) to avoid JSON key conflict.

## Convenience Functions

```bash
remotePull 10.0.0.1           # git pull automateLinux
remoteBd 10.0.0.1             # build daemon
remoteDeployDaemon 10.0.0.1   # pull + build daemon
```

## Peer Network

| Peer | IP | Role |
|------|-----|------|
| Desktop | 10.0.0.2 | Leader |
| VPS | 10.0.0.1 (`$kamateraIp`) | Worker |
| Laptop | 10.0.0.4 | Worker |

## Troubleshooting

- **"Peer not found"**: Run `d listPeers` - peer must register with leader first
- **"Failed to connect"**: Check `wg show`, `ping <ip>`, `systemctl status daemon.service`
- **Peer unreachable/needs bootstrap**: Ask the user to manually set it up. Never use SSH.
