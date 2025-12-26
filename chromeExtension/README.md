# AutomateLinux Chrome Extension - Active Tab Tracker

This Chrome extension tracks the currently active tab and sends its URL to the AutomateLinux daemon via WebSocket.

## Installation

1. Open Chrome and go to `chrome://extensions/`
2. Enable "Developer mode" (toggle in top right)
3. Click "Load unpacked"
4. Select this directory (`/home/yaniv/coding/automateLinux/chromeExtension`)

## How It Works

- Listens for tab activation and URL changes
- Connects to daemon's WebSocket server at `ws://localhost:9223`
- Sends active tab URL in real-time
- Automatically reconnects if connection is lost

## Permissions

- `tabs`: Required to query active tab information
- `host_permissions`: Required to access tab URLs

## Files

- `manifest.json`: Extension configuration
- `background.js`: Service worker that tracks tabs and communicates with daemon
