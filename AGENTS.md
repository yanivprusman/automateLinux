# automateLinux Agent Instructions

## Daemon Function

### Do NOT modify the daemon() function
The `daemon()` function in `/home/yaniv/coding/automateLinux/terminal/functions/daemon.sh` is the core IPC bridge to the C++ daemon process. 

**Important:** Do not change its signature, JSON format, or communication protocol.

If you need to add functionality that uses daemon commands:
1. Create new wrapper functions (not modifications to `daemon()` itself)
2. These wrappers can parse and format the output from `daemon()`
3. Examples: `dirHistoryShowIndex()`, `dirHistoryListAll()`, etc.

This keeps the core IPC mechanism stable and testable.

## Directory History System

The daemon maintains a directory history database with:
- Key format: `dirHistory0|<index>` for entries
- Key format: `pointerDevPtsN` for terminal pointers
- The insertAt mechanism shifts subsequent pointers when inserting at an index

When implementing new features:
1. Use wrapper functions around `daemon` commands
2. Never modify the JSON protocol directly in daemon.sh
3. Handle multiline responses - they are JSON-escaped with `\n` literals

## Legacy Code - Do NOT Modify

### dirHistory.sh
The file `/home/yaniv/coding/automateLinux/terminal/functions/dirHistory.sh` contains legacy file-based directory history implementation and will be deleted in a future refactor.

**Do NOT add any new code to this file.** The following functions are legacy and should not be modified:
- `resetWithDefaultDir()`
- `getEscapedTty()`
- `testIfProper()`
- `resetDirHistoryToBeginningState()`
- `initializeDirHistory()`
- `cdToPointer()`
- `insertDir()`
- `insertDirAtIndex()`
- `insertDirAfterIndex()`
- `pd()` - old backward navigation
- `pdd()` - old forward navigation
- `setDirHistoryPointer()`
- `getDirHistoryPointer()`
- `getDirFromHistory()`
- `resetDirHistoryToBeginningStateIfError()`
- `updateDirHistory()` - legacy version

**New Features:** Add daemon wrapper functions in a new dedicated file instead. Current daemon wrappers are:
- `dirHistoryShowIndex()`
- `dirHistoryListAll()`
- `dirHistoryDeleteAll()`
- `dirHistoryForward()`
- `dirHistoryBackward()`

