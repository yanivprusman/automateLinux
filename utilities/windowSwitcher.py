#!/usr/bin/env python3
import socket
import json
import sys
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

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
        sys.exit(1)

def main():
    print("Fetching windows...")
    resp = send_command({"command": "listWindows"})
    
    try:
        if resp.startswith("error:") or "Window extension not registered" in resp:
             print(f"Error: {resp}")
             print("Ensure GNOME extension is enabled and daemon is running.")
             return

        windows = json.loads(resp)
    except json.JSONDecodeError:
        print(f"Invalid response: {resp}")
        return

    if not windows:
        print("No windows found.")
        return

    print("\nSelect a window to activate:")
    for i, w in enumerate(windows):
        # Clean title
        title = w.get('title', 'Unknown')
        app = w.get('wm_class', 'Unknown App')
        workspace = w.get('workspace', '?')
        print(f"[{i}] {title} ({app}) [WS: {workspace}]")

    try:
        choice = input("\nEnter number (or q to quit): ")
        if choice.lower() == 'q':
            return
        idx = int(choice)
        if 0 <= idx < len(windows):
            target = windows[idx]
            print(f"Activating {target['title']}...")
            res = send_command({
                "command": "activateWindow", 
                "windowId": str(target['id']) 
                # Note: mainCommand receives "command": "activateWindow"
                # and forwards "action": "activateWindow" with args.
            })
            print(f"Result: {res}")
        else:
            print("Invalid selection.")
    except ValueError:
        print("Invalid input.")

if __name__ == "__main__":
    main()
