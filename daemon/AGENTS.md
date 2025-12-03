inside classes and functions no empty lines.
do not add comments, but do not delete existing comments.
keep indenting code like:
std::string getTtyFromPid(pid_t pid) {
    std::string fdPath = "/proc/" + std::to_string(pid) + "/fd";
}
instead of spawing processes do coproc like in 
initCoproc in functions.sh

# Daemon IPC Relay Implementation Guide

## Problem Statement
Need to create a persistent bidirectional relay between bash FIFOs and a Unix domain socket for daemon IPC communication without blocking shell startup or consuming excessive resources.

## Design Constraints
1. **No blocking on shell startup** 
2. **True bidirectional** - both IN and OUT FIFOs must be connected simultaneously
3. **Single persistent connection** - not reconnecting per request (performance)
4. **No forking** - single process, not spawning children per connection
5. **Performant** - minimal CPU/memory when idle
6. **Non-intrusive** - shell initialization must complete immediately

## Rejected Approaches

### ❌ Option 1: Direct Bash Socket Connection
**Code**: `exec 3<>/run/automatelinux/automatelinux-daemon.sock`
**Problem**: Bash `exec` cannot handle SOCK_STREAM; results in "No such device or address" errors.

### ❌ Option 2: Single nc with Pipe
**Code**: `cat "$IN" | nc -U "$SOCKET" > "$OUT" &`
**Problem**: nc exits after connection closes; relay dies after first request.

### ❌ Option 3: nc in while loop
**Code**: `while true; do cat "$IN" | nc -U "$SOCKET" > "$OUT"; done`
**Problem**: Reconnects per request (slow), socket resource inefficient.

### ❌ Option 4: socat UNIX-CONNECT Blocking
**Code**: `socat FIFO:"$IN" UNIX-CONNECT:"$SOCKET" &`
**Problem**: UNIX-CONNECT blocks; shells hang if daemon not ready.

### ❌ Option 5: socat STDIO One-shot
**Code**: `(socat STDIO UNIX-CONNECT:"$SOCKET") < "$IN" > "$OUT" &`
**Problem**: Exits after first request (EOF on stdin).

### ❌ Option 6: socat with fork
**Code**: `socat FIFO:"$IN" UNIX-CONNECT:"$SOCKET",fork &`
**Problem**: `fork` creates new process per connection (memory overhead, context switching).

## ✅ Correct Solution: Persistent Socket with Loop

```bash
firstTime(){
    local AUTOMATE_LINUX_DAEMON_IN_FILE="${AUTOMATE_LINUX_DATA_DIR}relay_${AUTOMATE_LINUX_TTY_NUMBER}_in"
    local AUTOMATE_LINUX_DAEMON_OUT_FILE="${AUTOMATE_LINUX_DATA_DIR}relay_${AUTOMATE_LINUX_TTY_NUMBER}_out"
    rm -f "$AUTOMATE_LINUX_DAEMON_IN_FILE" "$AUTOMATE_LINUX_DAEMON_OUT_FILE" 2>/dev/null
    mkfifo "$AUTOMATE_LINUX_DAEMON_IN_FILE" "$AUTOMATE_LINUX_DAEMON_OUT_FILE" 2>/dev/null
    
    (
        exec 3<>"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null || exit 1
        while true; do
            stdbuf -o0 -i0 cat "$AUTOMATE_LINUX_DAEMON_IN_FILE" >&3
            stdbuf -o0 -i0 cat <&3 > "$AUTOMATE_LINUX_DAEMON_OUT_FILE"
        done
    ) &
    export AUTOMATE_LINUX_RELAY_PID=$!
    sleep 0.2
    
    exec {AUTOMATE_LINUX_DAEMON_IN_FD}>"$AUTOMATE_LINUX_DAEMON_IN_FILE"
    exec {AUTOMATE_LINUX_DAEMON_OUT_FD}<"$AUTOMATE_LINUX_DAEMON_OUT_FILE"
    
    trap "rm -f '$AUTOMATE_LINUX_DAEMON_IN_FILE' '$AUTOMATE_LINUX_DAEMON_OUT_FILE' 2>/dev/null; kill $AUTOMATE_LINUX_RELAY_PID 2>/dev/null" HUP
}
firstTime
unset firstTime