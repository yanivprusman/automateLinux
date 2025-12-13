1) learn about std::queue + std::mutex + std::thread
in daemon is there a more performant way to log?
can we open a file once at startup instead of multiple times? in the current version do we open multiple times? can we send the loging toa asinchronious process / parrellel / thread etc?

Let me analyze the current logging approach and suggest optimizations.

Let me check the async implementation and main.cpp structure:

Read main.cpp, lines 1 to 100

Now let me create a comprehensive plan for optimized logging:

Plan: Optimize Daemon Logging Performance
Currently, logging opens the file multiple times per command in handleSetKeyboard() (lines 228-229, and potentially multiple times if logFile.is_open() is true). This causes repeated file I/O overhead. Implement three optimizations: (1) open log file once at daemon startup and keep it open, (2) use a separate async logging thread to prevent blocking, and (3) use buffering to batch writes.

Steps
Create async logger infrastructure in new file daemon/include/logger.h and daemon/src/logger.cpp with: global logger instance, thread-safe queue for log messages, background worker thread that flushes messages to file.

Add global logger to daemon/src/main.cpp initialization (around line 80-90 in initialize() function) to open log file once at startup and spawn async logger thread.

Replace synchronous logging in daemon/src/mainCommand.cpp handleSetKeyboard() and other handlers to use logAsync(message) instead of repeatedly opening/closing file with ofstream.

Add signal handler for graceful shutdown to flush remaining logs before daemon exits.

Verify performance by comparing log timestamps before/after: should eliminate 10+ file open/close syscalls per keyboard toggle.

Further Considerations
Logging mechanism choice: (A) Use std::queue + std::mutex + std::thread for a dedicated logging thread that batches writes; (B) Use lock-free queue (moodycamel::ConcurrentQueue) if available; (C) Simple approach: just keep file open globally and use std::mutex for thread-safe writes without a separate thread.

Performance trade-off: Option A (async thread) is best for latency but adds thread overhead. Option C (global file + mutex) is simpler and still ~100x faster than reopening file each time. Recommend Option C unless logging frequency is very high.

Backwards compatibility: Current code passes ofstream& to readScriptFile() but doesn't actually use it. Should refactor to remove that parameter and simplify the interface.

Would you like me to proceed with implementation? I recommend Option C (keep file open + mutex) as the safest, simplest, and most performant for this use case.