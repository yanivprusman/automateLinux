---
name: peer-commands
description: Execute commands on remote peers via daemon, never use direct SSH
---

# Peer Command Execution

## Primary Method (ALWAYS USE THIS)

```bash
d execOnPeer --peer vps --directory /opt/automateLinux --shellCmd "git pull"
```

**Convenience functions:**
```bash
remotePull 10.0.0.1           # git pull automateLinux
remoteBd 10.0.0.1             # build daemon
remoteDeployDaemon 10.0.0.1   # pull + build daemon
```

## Bootstrap Exception (peer has old daemon)

When a peer needs the new daemon code before `execOnPeer` works, use the **wrapper functions only**:

```bash
# Source the functions first
source /opt/automateLinux/terminal/functions/sshgit.sh

# Then use sshCmd (handles directory automatically)
sshCmd root@10.0.0.1 /opt/automateLinux "git pull && cd daemon && source ./build.sh"
```

**NEVER use raw SSH:**
```bash
# WRONG - will fail with "not a git repository"
ssh 10.0.0.1 "git pull"
```

## Peer Network

| Peer | IP | Role |
|------|-----|------|
| VPS | 10.0.0.1 (`$kamateraIp`) | Leader |
| Desktop | 10.0.0.2 | Worker |
| Laptop | 10.0.0.4 | Worker |

## Troubleshooting

- **"Peer not found"**: Run `d listPeers` - peer must register with leader first
- **"Failed to connect"**: Check `wg show`, `ping <ip>`, `systemctl status daemon.service`
- **Bootstrap needed**: Use `sshCmd` wrapper (see above), never raw SSH
