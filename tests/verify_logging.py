
import socket
import json
import time
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

def send_command(cmd):
    try:
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(SOCKET_PATH)
        client.sendall(json.dumps(cmd).encode('utf-8') + b'\n')
        response = client.recv(4096).decode('utf-8')
        client.close()
        return response
    except Exception as e:
        print(f"Error: {e}")
        return None

def verify():
    # 1. Disable Logging
    print("Disabling logging...")
    send_command({"command": "shouldLog", "enable": "false"})
    
    # 2. Trigger Window Change
    print("Triggering window change (should not log)...")
    send_command({
        "command": "activeWindowChanged",
        "windowTitle": "Test Window",
        "wmClass": "test.class",
        "wmInstance": "test.instance",
        "windowId": 12345
    })
    
    time.sleep(1)
    
    # Check log file
    log_file = "/opt/automateLinux/data/combined.log"
    size_after_disabled = os.path.getsize(log_file)
    
    # 3. Enable Logging
    print("Enabling logging...")
    send_command({"command": "shouldLog", "enable": "true"})
    
    # 4. Trigger Window Change
    print("Triggering window change (should log)...")
    send_command({
        "command": "activeWindowChanged",
        "windowTitle": "Test Window 2",
        "wmClass": "test.class.2",
        "wmInstance": "test.instance.2",
        "windowId": 67890
    })
    
    time.sleep(1)
    
    size_after_enabled = os.path.getsize(log_file)
    
    if size_after_enabled > size_after_disabled:
        print("SUCCESS: Logging works as expected.")
        # Read last lines to confirm
        with open(log_file, 'r') as f:
            lines = f.readlines()
            print("Last log lines:")
            print("".join(lines[-3:]))
    else:
        print("FAILURE: No new logs found after enabling.")

if __name__ == "__main__":
    verify()
