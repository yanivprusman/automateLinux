#!/usr/bin/env python3
"""
Native messaging host for AutomateLinux Chrome extension.
Threaded Bidirectional version with enhanced logging and error handling.
"""

import sys
import json
import struct
import socket
import os
import threading
import time
from datetime import datetime

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"
LOG_FILE = "/tmp/com.automatelinux.native-host.log"

def get_timestamp():
    """Get ISO format timestamp matching extension logs."""
    return datetime.now().isoformat()

def log(message):
    """Log to file for debugging with timestamp."""
    try:
        with open(LOG_FILE, 'a') as f:
            f.write(f"[{get_timestamp()}] [NativeHost] {message}\n")
            f.flush()
    except Exception:
        # Avoid printing to stdout as it breaks the native messaging protocol
        pass

def send_to_chrome(message):
    """Send a message to Chrome extension through stdout."""
    try:
        if not isinstance(message, dict):
            log(f"Error: Message must be dict, got {type(message)}")
            return False
        
        encoded_message = json.dumps(message).encode('utf-8')
        sys.stdout.buffer.write(struct.pack('I', len(encoded_message)))
        sys.stdout.buffer.write(encoded_message)
        sys.stdout.buffer.flush()
        
        # Log only action/command, not full message to avoid log spam
        msg_type = message.get('action') or message.get('command') or 'unknown'
        log(f"→ Chrome: {msg_type} (seq: {message.get('seq', 'N/A')})")
        return True
    except Exception as e:
        log(f"Error sending to chrome: {e}")
        return False

def read_from_chrome():
    """Read a message from Chrome extension through stdin."""
    try:
        raw_length = sys.stdin.buffer.read(4)
        if not raw_length:
            return None
        message_length = struct.unpack('I', raw_length)[0]
        
        # Sanity check message length (max 10MB)
        if message_length > 10 * 1024 * 1024:
            log(f"Error: Message too large: {message_length} bytes")
            return None
            
        message = sys.stdin.buffer.read(message_length).decode('utf-8')
        parsed = json.loads(message)
        
        # Log only essential info to avoid spam
        msg_type = parsed.get('action') or parsed.get('command') or parsed.get('url', '').split('/')[-1][:30]
        log(f"← Chrome: {msg_type} (seq: {parsed.get('seq', 'N/A')})")
        return parsed
    except Exception as e:
        log(f"Error reading from chrome: {e}")
        return None

class DaemonLink:
    def __init__(self):
        self.sock = None
        self.running = True
        self.lock = threading.Lock()

    def connect(self):
        with self.lock:
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
            try:
                self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.sock.connect(SOCKET_PATH)
                # Register as native host
                register_cmd = json.dumps({"command": "registerNativeHost"}) + "\n"
                self.sock.sendall(register_cmd.encode('utf-8'))
                log("Connected and registered with daemon")
                return True
            except Exception as e:
                log(f"Error connecting to daemon: {e}")
                self.sock = None
                return False

    def send(self, data_dict):
        with self.lock:
            if not self.sock:
                if not self.connect():
                    return False
            try:
                cmd = json.dumps(data_dict) + "\n"
                self.sock.sendall(cmd.encode('utf-8'))
                return True
            except Exception as e:
                log(f"Error sending to daemon: {e}")
                self.sock = None
                return False

    def listen_loop(self):
        log("Daemon listener thread started")
        buffer = ""
        while self.running:
            if not self.sock:
                time.sleep(1)
                self.connect()
                continue
            
            try:
                data = self.sock.recv(4096).decode('utf-8')
                if not data:
                    log("Daemon disconnected")
                    self.sock = None
                    continue
                
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    if not line.strip():
                        continue
                    try:
                        msg = json.loads(line)
                        send_to_chrome(msg)
                    except Exception as e:
                        log(f"Error parsing daemon message: {e} | Line: {line}")
            except Exception as e:
                log(f"Error in daemon listen loop: {e}")
                self.sock = None
                time.sleep(1)

def main():
    log("Native host main starting")
    try:
        link = DaemonLink()
        log("DaemonLink initialized")
        
        # Start listener thread for daemon -> chrome
        listener = threading.Thread(target=link.listen_loop, daemon=True)
        listener.start()
        log("Daemon listener thread started")

        # Main loop for chrome -> daemon
        log("Entering main message loop")
        while True:
            message = read_from_chrome()
            if message is None:
                log("Chrome disconnected (read None), exiting")
                link.running = False
                break
        
            # log(f"From Chrome: {message}")
            if isinstance(message, dict):
                # If the message already has a 'command', use it. 
                # Otherwise, check for 'url' (legacy/specific case)
                if 'command' in message:
                    link.send(message)
                elif 'url' in message:
                    link.send({"command": "setActiveTabUrl", "url": message['url']})
                elif 'action' in message:
                    # Handle focusAck and other 'action' style messages from extension
                    if message['action'] == 'focusAck':
                        link.send({"command": "focusAck"})
                    else:
                        # Forward any other action as is? Maybe wrap it?
                        # For now, if it's an action we don't recognize as a daemon command,
                        # we might need to map it.
                        log(f"Unknown action from chrome: {message['action']}")
            else:
                log(f"Unsupported message type from Chrome: {type(message)}")

    except Exception as e:
        log(f"CRITICAL: exception in main loop: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
