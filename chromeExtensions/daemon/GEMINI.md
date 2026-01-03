# Project Overview

This is a Chrome extension that tracks the active tab's URL and sends it to a local daemon. It also listens for commands from the daemon, such as focusing the ChatGPT input field.

The extension consists of a background service worker (`background.js`) and a content script (`content.js`). The background script handles tab tracking and communication with the local daemon. The content script is injected into `chatgpt.com` to allow the extension to interact with the page.

Communication with the daemon is currently implemented using an HTTP bridge and Server-Sent Events (SSE). The extension sends the active tab URL to `http://localhost:9223/active-tab` and listens for events from `http://localhost:9223/events`.

The project also contains files for a native messaging host (`native-host.py`, `com.automatelinux.tabtracker.json`, `native-host-wrapper.sh`), which appears to be a previous or alternative method of communication with the daemon.

## Building and Running

To run this extension:

1.  Open Chrome and navigate to `chrome://extensions`.
2.  Enable "Developer mode".
3.  Click "Load unpacked" and select the directory containing this extension.

The extension will start running immediately. It expects a local daemon to be running and listening on `http://localhost:9223`.

## Development Conventions

The codebase is written in plain JavaScript. The background script uses async/await for handling asynchronous operations.

The extension follows the Manifest V3 specification.

The primary communication mechanism is via HTTP and SSE, not native messaging. The native messaging files are likely deprecated but are kept in the repository. Any new development should likely focus on the HTTP/SSE bridge.

## Critical Feature: Ctrl+V Focus+Paste on ChatGPT

### Behavior Requirements

When the user is on Chrome with a tab URL starting with `https://chatgpt.com` and presses `Ctrl+V`:

1. **Automatic Focus**: The ChatGPT textarea must receive focus automatically (if not already focused)
2. **Single Paste**: Paste operation happens once (not twice - once for focus, once for paste)
3. **Speed**: Complete flow must execute in <200ms for optimal UX
4. **Native Messaging**: Communication must use Native Messaging (Chrome ↔ daemon)

### Communication Flow

```
User presses Ctrl+V in Chrome
  ↓
Daemon detects keypress (InputMapper.cpp)
  ↓
Daemon sends focus command via native messaging
  ↓
Native host (native-host.py) forwards to extension
  ↓
Extension (background.js) focuses textarea
  ↓
Extension sends ACK back
  ↓
Daemon receives ACK and triggers paste
  ↓
Paste completes in focused textarea
```

### Automated Testing Requirements

Every test must validate the complete flow without manual intervention:

1. **Reload Extension**: `./reload-extension.sh`
2. **Navigate to ChatGPT**: Open or switch to `https://chatgpt.com` tab
3. **Defocus Textarea**: Click elsewhere on page to ensure textarea not focused
4. **Simulate Ctrl+V**: Trigger actual keypress via evemu or similar
5. **Verify Focus**: Check that textarea received focus
6. **Verify Paste**: Confirm clipboard content appeared in textarea
7. **Measure Timing**: Total time must be <200ms
8. **Handle Verification**: Auto-handle "verify you are human" prompts if present

### Test Scripts

- `/home/yaniv/coding/automateLinux/test-focus-paste-e2e.sh` - Semi-automated with manual verification
- `/home/yaniv/coding/automateLinux/test-fully-automated.py` - Fully automated using evemu
- `/home/yaniv/coding/automateLinux/test-real-world.sh` - Real-world manual trigger test

### Current Status

- ✅ ACK timeout issue resolved (extension always sends ACK)
- ✅ Focus command working (<100ms latency)
- ✅ Native messaging communication verified
- ✅ Paste integration working
- ⚠️ Full automation test pending (Cloudflare handling needed)

### Key Files

- `background.js` - Enhanced with timestamps, sequences, ACK fix
- `native-host.py` - Python bridge between Chrome and daemon
- `daemon/src/InputMapper.cpp` - Detects Ctrl+V, triggers focus
- `daemon/src/mainCommand.cpp` - Handles native host registration, focus commands
