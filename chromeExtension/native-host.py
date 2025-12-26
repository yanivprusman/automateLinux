#!/usr/bin/env python3
"""
Native messaging host for AutomateLinux Chrome extension.
Receives active tab URL from Chrome and sends it to daemon via UDS.
"""

import sys
import json
import struct
import socket

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

def send_message(message):
    """Send a message to Chrome extension."""
    encoded_message = json.dumps(message).encode('utf-8')
    sys.stdout.buffer.write(struct.pack('I', len(encoded_message)))
    sys.stdout.buffer.write(encoded_message)
    sys.stdout.buffer.flush()

def read_message():
    """Read a message from Chrome extension."""
    raw_length = sys.stdin.buffer.read(4)
    if not raw_length:
        return None
    message_length = struct.unpack('I', raw_length)[0]
    message = sys.stdin.buffer.read(message_length).decode('utf-8')
    return json.loads(message)

def send_to_daemon(url):
    """Send active tab URL to daemon via UDS."""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        
        # Send command to update active tab URL
        command = json.dumps({"command": "setActiveTabUrl", "url": url}) + "\n"
        sock.sendall(command.encode('utf-8'))
        
        sock.close()
        return True
    except Exception as e:
        # Log error but don't crash
        sys.stderr.write(f"Error sending to daemon: {e}\n")
        sys.stderr.flush()
        return False

def main():
    """Main loop for native messaging host."""
    while True:
        message = read_message()
        if message is None:
            break
        
        # Extract URL from message
        url = message.get('url', '')
        
        # Send URL to daemon
        success = send_to_daemon(url)
        
        # Send acknowledgment back to extension
        send_message({"status": "ok" if success else "error", "url": url})

if __name__ == '__main__':
    main()
