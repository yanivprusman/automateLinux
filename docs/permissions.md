# Permissions Design

How automateLinux manages file ownership, access control, and privilege escalation across machines.

## Overview

All machines use a shared `coding` group. Files are owned by `root:coding` with group-write enabled. This lets any user in the `coding` group build, deploy, and modify project files without fighting ownership issues.

## Layers

### 1. Ownership: `root:coding`

All project directories use `root:coding` ownership:

```
/opt/automateLinux/       root:coding
/opt/dev/<app>/           root:coding
/opt/prod/<app>/          root:coding
```

Root owns the files (daemon runs as root via systemd). The `coding` group grants write access to human users.

### 2. Setgid on Directories (g+s / mode 2xxx)

Every directory has the setgid bit set. This ensures new files and subdirectories inherit the `coding` group, regardless of which user creates them.

```bash
# Applied by install.sh and fixAutoLinuxPerms:
find "$DIR" -type d -exec chmod g+s {} \;

# Specific directories use octal 2775:
chmod -R 2775 /opt/automateLinux/data
chmod -R 2775 /opt/automateLinux/config
```

Without setgid, files created by `root` would be `root:root` and files by `yaniv` would be `yaniv:yaniv` — breaking shared access.

### 3. umask 002 (System-Wide via PAM)

System-wide umask set to `002` so new files are group-writable by default:

```
# /etc/pam.d/common-session
session optional pam_umask.so umask=002
```

Default umask `022` strips group-write. With `002`, new files get `664` (rw-rw-r--) and directories get `775` (rwxrwxr-x).

### 4. Sudoers Rules (`/etc/sudoers.d/codingUsers`)

The `coding` group gets passwordless sudo for specific commands. Deployed by `install.sh` from `sudoers/codingUsers`:

| Category | Commands |
|----------|----------|
| Config management | chown, chmod, setfacl on `/home/*/.config` |
| OverlayFS | mount overlay, umount for Chrome/VS Code |
| User management | groupadd, useradd, usermod, deluser, chpasswd |
| Session management | pkill, systemctl stop/start accounts-daemon |
| File operations | mkdir, rm, ln, cp, touch, tee in `/home/*` |
| System | reboot, systemctl restart daemon.service, install.sh |

The daemon itself runs as root (systemd), so it doesn't need sudoers rules. These rules are for interactive terminal use.

### 5. ACLs (setfacl) — Shared User Configs

For sharing `.config` directories between users (multi-user desktop), ACLs provide fine-grained control beyond traditional UNIX permissions:

```bash
# Default ACLs on shared dirs — new files auto-inherit permissions:
setfacl -R -d -m "u:yaniv:rwx,g:coding:rwx,m:rwx" "$DIR"

# Mask management — prevents Chrome/apps from stripping group perms:
setfacl -R -m "m:rwx" "$DIR"
```

ACLs are used specifically for:
- `/home/<user>/.config` sharing between users
- Daemon socket directory (`/run/automatelinux/`)
- Chrome profile directory (which aggressively resets permissions)

### 6. Daemon Socket Permissions

The UNIX socket needs to be accessible by non-root users:

```bash
# Socket directory:
chmod g+rws /run/automatelinux
setfacl -m "g:coding:rx,m:rwx" /run/automatelinux

# Socket file:
chmod 0666 /run/automatelinux/automatelinux-daemon.sock
```

The socket is world-readable/writable (`0666`) since access control is handled at the application level, not filesystem level.

## Helper Functions

| Function | Purpose |
|----------|---------|
| `fixAutoLinuxPerms [path]` | Fix ownership to `root:coding`, group-writable, setgid on dirs |
| `_tus` / `_tuShareConfig` | Set up ACLs for sharing .config between users |
| `_tuTakeConfig` | Take ownership of .config for current user |
| `_tuFixConfig` | Restore group permissions on .config |
| `_tuFixChrome` | Fix Chrome profile permissions (deletes locks, resets perms) |

## New Machine Setup

On a fresh machine, `install.sh` handles most of this automatically:

1. Creates `coding` group
2. Adds the installing user to `coding`
3. Sets `root:coding` ownership on `/opt/automateLinux`
4. Enables group-write + setgid on all directories
5. Deploys sudoers rules to `/etc/sudoers.d/codingUsers`
6. Sets `2775` on data/config directories

**Manual steps still needed:**
- Set system-wide umask: add `session optional pam_umask.so umask=002` to `/etc/pam.d/common-session`
- Log out and back in for group membership to take effect
- For extra apps: `fixAutoLinuxPerms /opt/dev/<app>` and `fixAutoLinuxPerms /opt/prod/<app>`

## What's NOT Used

- **Sticky bit (+t)**: Not needed. All `coding` group members are trusted equally — no need to prevent cross-deletion.
- **Capabilities**: The daemon runs as root; no need for fine-grained capabilities.
- **SELinux/AppArmor**: Not configured. Standard UNIX permissions + ACLs are sufficient for this use case.
