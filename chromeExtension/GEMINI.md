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
