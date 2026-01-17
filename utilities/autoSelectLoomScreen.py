#!/usr/bin/env python3
import socket
import json
import time
import sys
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

# Key mappings
KEY_MAP = {
    "TAB": 15,
    "ENTER": 28,
    "SPACE": 57,
    "RIGHT": 106,
    "LEFT": 105,
    "UP": 103,
    "DOWN": 108,
    "ESC": 1,
}

def send_command(cmd_obj):
    try:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(SOCKET_PATH)
        msg = json.dumps(cmd_obj) + "\n"
        s.sendall(msg.encode('utf-8'))
        
        # Read response
        response = ""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            response += chunk.decode('utf-8')
            if response.endswith("\n"):
                break
        s.close()
        return response.strip()
    except Exception as e:
        print(f"Error communicating with daemon: {e}")
        return None

def simulate_key(code):
    # Press
    send_command({"command": "simulateInput", "type": 1, "code": code, "value": 1})
    time.sleep(0.05)
    # Release
    send_command({"command": "simulateInput", "type": 1, "code": code, "value": 0})

def main():
    # Parse arguments for custom sequence
    if len(sys.argv) > 1:
        # Join all args to handle "TAB TAB" or TAB TAB
        sequence_str = " ".join(sys.argv[1:])
        # Split by keys
        # Handle comma or space separation
        sequence_list = sequence_str.replace(',', ' ').split()
    else:
        # Default sequence
        # Updated based on user hypothesis: TAB TAB ENTER TAB TAB ENTER
        sequence_list = ["TAB", "TAB", "ENTER", "TAB", "TAB", "ENTER"]

    print(f"Using key sequence: {sequence_list}")

    print("Waiting for 'Share Screen' window...")
    found = False
    window_id = None
    
    # Retry loop: wait up to 15 seconds
    start_time = time.time()
    while time.time() - start_time < 15:
        resp = send_command({"command": "listWindows"})
        if resp:
            try:
                windows = json.loads(resp)
                for w in windows:
                    title = w.get('title', '')
                    app = w.get('wm_class', '')
                    # Look for the portal window
                    if "Share Screen" in title or ("xdg-desktop-portal" in app and "Share" in title):
                         window_id = w['id']
                         found = True
                         break
            except:
                pass
        
        if found:
            break
        
        # Debug: Print windows seen if not found (throttle to once per second?)
        if int(time.time()) % 2 == 0:
             print(f"Scanning... Windows found: {[w.get('title', 'Unknown') for w in windows]}")

        time.sleep(0.5)

    if not found:
        print("Window not found in list. Assuming it is a focused system modal. Attempting blind selection.")
        # Proceed without activating window (assume focused)
        window_id = "BLIND"

    if found and window_id:
        print(f"Found window {window_id}. Activating...")
        send_command({"command": "activateWindow", "windowId": str(window_id)})
        # Wait for window to be focused
        time.sleep(0.5)

    print("Sending selection keys...")
    
    for key_name in sequence_list:
        key_upper = key_name.upper()
        if key_upper in KEY_MAP:
            print(f"Processing {key_upper}")
            simulate_key(KEY_MAP[key_upper])
            time.sleep(0.2)
        else:
            print(f"WARNING: Unknown key '{key_name}', skipping.")
    
    print("Done.")

if __name__ == "__main__":
    main()
