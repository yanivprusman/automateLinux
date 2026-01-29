# SSH Git Operations Skill

## Skill Name
`sshgit`

## Description
This skill handles git operations on remote servers via SSH, automatically handling the critical requirement to change directory before executing git commands.

## Problem Solved
**SSH commands start in the user's home directory, not the target git repository.** Running `ssh user@host 'git pull'` will fail with "not a git repository" error. The command must include `cd /path/to/repo && git command`.

## When to Use This Skill

Use this skill whenever you need to:
- Run git commands on remote servers (pull, status, log, etc.)
- Execute any command in a specific directory on a remote server
- Interact with the VPS (10.0.0.1) automateLinux installation
- Interact with the VPS CAD repository

## Available Functions

### Core Functions

#### `sshGit <user@host> <directory> <git-command>`
Execute any git command in a remote directory.

**Examples:**
```bash
sshGit root@10.0.0.1 /opt/automateLinux pull
sshGit root@10.0.0.1 /opt/automateLinux status
sshGit yaniv@10.0.0.1 ~/cad-prod log --oneline -5
```

#### `sshCmd <user@host> <directory> <command>`
Execute any command in a remote directory (not just git).

**Examples:**
```bash
sshCmd root@10.0.0.1 /opt/automateLinux ./install.sh
sshCmd yaniv@10.0.0.1 ~/cad-prod ./build.sh
sshCmd root@10.0.0.1 /opt/automateLinux ls -la
```

### Convenience Functions

#### `vpsPull`
Pull automateLinux changes on VPS.
```bash
vpsPull
# Equivalent to: sshGit root@10.0.0.1 /opt/automateLinux pull
```

#### `vpsCADPull`
Pull CAD repository changes on VPS.
```bash
vpsCADPull
# Equivalent to: sshGit yaniv@10.0.0.1 ~/cad-prod pull
```

## How to Invoke

### For Claude Code (AI Assistant)
When you need to run git commands on remote servers, follow this pattern:

1. Source the skill functions (done automatically in bashrc):
```bash
source /opt/automateLinux/terminal/functions/sshgit.sh
```

2. Use the appropriate function instead of raw SSH commands:

**❌ WRONG:**
```bash
ssh root@10.0.0.1 'git pull'  # FAILS - not in git repo
```

**✅ CORRECT:**
```bash
vpsPull  # or
sshGit root@10.0.0.1 /opt/automateLinux pull
```

### For Human Users
These functions are automatically available after running `user_install.sh` as they are sourced in the terminal bashrc.

## Common Workflows

### Update VPS AutomateLinux Installation
```bash
# 1. Push changes from desktop
git add file.sh
git commit -m "Update file"
git push

# 2. Pull on VPS and reinstall
vpsPull
sshCmd root@10.0.0.1 /opt/automateLinux './install.sh --minimal'
```

### Update VPS CAD Deployment
```bash
# Pull latest CAD code
vpsCADPull

# Rebuild
sshCmd yaniv@10.0.0.1 ~/cad-prod './build.sh'
```

### Check Status on VPS
```bash
sshGit root@10.0.0.1 /opt/automateLinux status
sshGit root@10.0.0.1 /opt/automateLinux log --oneline -5
```

## Technical Implementation

Located at: `/opt/automateLinux/terminal/functions/sshgit.sh`

The functions work by:
1. Taking directory path as a required parameter
2. Constructing the SSH command as: `ssh user@host "cd directory && command"`
3. The `cd directory &&` is automatically prepended, preventing the common error

## Error Prevention

This skill was created to prevent a recurring pattern where Claude Code would repeatedly execute:
```bash
ssh root@10.0.0.1 'git pull'
```
Without the required `cd` command, causing continuous failures.

By encapsulating the pattern in functions, the skill:
- **Enforces** correct command structure
- **Prevents** forgetting the `cd` step
- **Makes** the pattern reusable and self-documenting
- **Reduces** cognitive load on both AI and human users

## Related Documentation

See also:
- `/opt/automateLinux/terminal/functions/sshgit.sh` - Implementation
- CLAUDE.md - General project guidance
- VPS connection details in wireguard.sh (10.0.0.1)
