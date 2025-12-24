import socket
import json
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

def send_command(command):
    if not os.path.exists(SOCKET_PATH):
        print(f"Socket {SOCKET_PATH} not found")
        return None
    
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(SOCKET_PATH)
        s.sendall(json.dumps(command).encode() + b"\n")
        response = s.recv(1024)
        return response.decode()

# 1. Disable logging
print("Disabling logging...")
print(send_command({"command": "shouldLog", "enable": "false"}))

# 2. Get logging status
print("Logging status:", send_command({"command": "getShouldLog"}))

# 3. Send activeWindowChanged (windowId as number)
print("Sending activeWindowChanged...")
print(send_command({
    "command": "activeWindowChanged",
    "windowTitle": "Python Test Title",
    "wmClass": "python-test",
    "wmInstance": "python-test",
    "windowId": 12345
}))
