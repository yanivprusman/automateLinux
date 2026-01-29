# Terminal Functions Skills

## Daemon Functions (daemon.sh)

### Remote Peer Execution

Execute commands on remote peers connected via WireGuard VPN.

| Function | Usage | Description |
|----------|-------|-------------|
| `getPeerIdByIp` | `getPeerIdByIp 10.0.0.1` | Lookup peer_id from IP |
| `execOnPeerByIp` | `execOnPeerByIp <ip> <dir> <cmd>` | Run command on peer |
| `remotePull` | `remotePull 10.0.0.1` | Git pull on remote peer |
| `remoteBd` | `remoteBd 10.0.0.1` | Build daemon on remote peer |
| `remoteDeployDaemon` | `remoteDeployDaemon 10.0.0.1` | Pull + build on peer |

### Example Workflow

```bash
# After pushing changes locally, deploy to VPS
git push
remoteDeployDaemon $kamateraIp
```

---

# Git Functions Skills

## gitCommitsToDirs

Creates separate directories for each commit in a repository's history, enabling side-by-side comparison of code evolution.

### Usage

```bash
gitCommitsToDirs <git-url> [branch]
```

### Parameters

- `git-url` (required): URL of the git repository to clone
- `branch` (optional): Branch to use. Defaults to `main`

### How It Works

1. Clones the repository with `--no-checkout` (metadata only)
2. Gets all commits in chronological order (oldest first) using `git rev-list --reverse`
3. Creates a git worktree for each commit as `{repo}_{n}/` where n is 1, 2, 3...
4. Each worktree is a full checkout at that specific commit

### Example

```bash
gitCommitsToDirs https://github.com/user/project.git main
# Creates:
#   project/        (bare clone)
#   project_1/      (first commit)
#   project_2/      (second commit)
#   project_3/      (third commit)
#   ...
```

### Local Repository Variant

For comparing commits in an existing local repo, use git worktree directly:

```bash
# Create worktrees for specific commits
git worktree add /tmp/project_original abc123
git worktree add /tmp/project_current def456

# Compare files
diff /tmp/project_original/src/file.cpp /tmp/project_current/src/file.cpp

# Clean up when done
git worktree remove /tmp/project_original
git worktree remove /tmp/project_current
```

### Use Cases

- **Code archaeology**: Trace how a feature evolved over time
- **Debugging regressions**: Find when a bug was introduced by comparing commits
- **Refactoring review**: Compare before/after states of major changes
- **Learning**: Study how a project was built incrementally
