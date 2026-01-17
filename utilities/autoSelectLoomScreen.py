#!/usr/bin/env python3
import socket
import json
import time
import sys
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"
KEY_TAB = 15
KEY_ENTER = 28
KEY_RIGHT = 106
KEY_SPACE = 57

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
        time.sleep(0.5)

    if not found:
        print("Window not found. Timed out.")
        return

    print(f"Found window {window_id}. Activating...")
    send_command({"command": "activateWindow", "windowId": str(window_id)})
    
    # Wait for window to be focused
    time.sleep(0.5)

    print("Sending selection keys...")
    # NOTE: This sequence logic depends on the exact behavior of xdg-desktop-portal-gnome.
    # Typically:
    # 1. Focus starts on the tab selector (Entire Screen / Window) or Cancel button.
    # 2. Press Tab to move to Main Content (Screen list).
    # 3. Press Space/Enter to select the first screen (if not selected).
    # 4. Press Tab to move to Share button.
    # 5. Press Enter to Share.
    
    # Attempt 1:
    # Tab -> Focus Screens
    simulate_key(KEY_TAB)
    time.sleep(0.2)
    
    # Select first screen (Space)
    simulate_key(KEY_SPACE)
    time.sleep(0.2)
    
    # Tab -> Focus "Share"
    simulate_key(KEY_TAB)
    time.sleep(0.2)
    
    # Enter -> Click "Share"
    simulate_key(KEY_ENTER)
    
    print("Done.")

if __name__ == "__main__":
    main()
