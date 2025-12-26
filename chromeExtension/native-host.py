#!/usr/bin/python3
"""
Native messaging host for AutomateLinux Chrome extension.
Threaded Bidirectional version.
"""

import sys
import json
import struct
import socket
import os
import threading
import time

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"
LOG_FILE = "/tmp/chrome-native-host.log"

def log(message):
    """Log to file for debugging."""
    try:
        with open(LOG_FILE, 'a') as f:
            f.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] {message}\n")
            f.flush()
    except:
        pass

def send_to_chrome(message):
    """Send a message to Chrome extension through stdout."""
    try:
        encoded_message = json.dumps(message).encode('utf-8')
        sys.stdout.buffer.write(struct.pack('I', len(encoded_message)))
        sys.stdout.buffer.write(encoded_message)
        sys.stdout.buffer.flush()
    except Exception as e:
        log(f"Error sending to chrome: {e}")

def read_from_chrome():
    """Read a message from Chrome extension through stdin."""
    try:
        raw_length = sys.stdin.buffer.read(4)
        if not raw_length:
            return None
        message_length = struct.unpack('I', raw_length)[0]
        message = sys.stdin.buffer.read(message_length).decode('utf-8')
        return json.loads(message)
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
    link = DaemonLink()
    link.connect()

    # Start listener thread for daemon -> chrome
    listener = threading.Thread(target=link.listen_loop, daemon=True)
    listener.start()

    # Main loop for chrome -> daemon
    while True:
        message = read_from_chrome()
        if message is None:
            log("Chrome disconnected, exiting")
            link.running = False
            break
        
        log(f"From Chrome: {message}")
        url = message.get('url', '')
        if url:
            link.send({"command": "setActiveTabUrl", "url": url})

if __name__ == '__main__':
    main()
