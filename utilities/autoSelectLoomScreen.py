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
    "LEFTSHIFT": 42,
    "LEFTALT": 56,
    "S": 31,
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
        print(f"Daemon response: {response.strip()}")
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
        # Added SPACE to ensure selection before TABbing to Share button
        sequence_list = ["SPACE", "TAB", "TAB", "ENTER"]

    print(f"Using key sequence: {sequence_list}")

    print("Waiting for 'Share Screen' window...")
    found = False
    window_id = None
    
    # Retry loop: wait up to 15 seconds
    start_time = time.time()
    while time.time() - start_time < 15:
        resp = send_command({"command": "listWindows"})
        print(f"Raw daemon response: {resp}")
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
        print("Window 'Share Screen' not found within timeout. Aborting.")
        return

    if found and window_id:
        print("Sending shift-nudge to wake up desktop focus...")
        # Shift Nudge
        simulate_key(KEY_MAP["LEFTSHIFT"])
        time.sleep(0.5)

        print(f"Found window {window_id}. Activating...")

        print(f"Activating window {window_id} (Portal)...")
        # Try activating multiple times to be sure
        for i in range(3):
            send_command({"command": "activateWindow", "windowId": str(window_id)})
            time.sleep(0.5)
        
        # Initial sleep for GNOME animations
        time.sleep(1.0)

    print("Starting robust key sequence...")
    
    # robust_sequence = [
    #     ("TAB", 0.2), # Focus into the area
    #     ("UP", 0.1),  ("UP", 0.1),  ("UP", 0.1), # Reset to top
    #     ("LEFT", 0.1), ("LEFT", 0.1), ("LEFT", 0.1), # Reset to left
    #     ("SPACE", 0.2), # Select first item
    #     ("ENTER", 0.2), # Confirm selection (sometimes needed)
    #     ("TAB", 0.2), # Move to Cancel
    #     ("TAB", 0.2), # Move to Share
    #     ("ENTER", 0.2) # Click Share
    # ]
    
    # Step 1: Ensure focus is on the content area
    print("Step 1: Focus content area (TAB)")
    simulate_key(KEY_MAP["TAB"])
    time.sleep(0.3)

    # Step 2: Reset selection to top-left (Screen 1)
    # Sending multiple UP/LEFTs safely moves to the first item even if already there
    print("Step 2: Reset selection to top-left")
    for _ in range(3):
        simulate_key(KEY_MAP["UP"])
        time.sleep(0.1)
    for _ in range(3):
        simulate_key(KEY_MAP["LEFT"])
        time.sleep(0.1)
    
    # Step 3: Select the item
    print("Step 3: Select item (SPACE + ENTER)")
    simulate_key(KEY_MAP["SPACE"])
    time.sleep(0.3)
    # Some versions might default to 'Share' button disabled until selection is confirmed?
    # Or SPACE just selects it. Let's add ENTER to be safe, though it might trigger 'Share' if already focused?
    # Usually SPACE toggles selection. 
    
    # Step 4: Kitchen Sink Strategy
    # We will try multiple triggers in sequence until one works.
    print("Step 4: triggering Share (Kitchen Sink)")
    
    # Configure slower key press
    def simulate_key_slow(code):
         send_command({"command": "simulateInput", "type": 1, "code": code, "value": 1})
         time.sleep(0.15) # Hold for 150ms
         send_command({"command": "simulateInput", "type": 1, "code": code, "value": 0})
         time.sleep(0.15)

    def check_closed():
        resp = send_command({"command": "listWindows"})
        if resp:
            try:
                wins = json.loads(resp)
                for w in wins:
                    if w['id'] == window_id:
                        return False
            except: 
                pass
        return True

    # Step 4: Polite Helper (Try once, then wait for user)
    print("Step 4: Polite Helper")
    
    # 1. Reset focus
    if window_id:
         send_command({"command": "activateWindow", "windowId": str(window_id)})
         time.sleep(0.5)

    # 2. Try simple trigger (Space + Enter)
    print("  Attempting simple trigger...")
    simulate_key_slow(KEY_MAP["SPACE"])
    time.sleep(0.5)
    simulate_key_slow(KEY_MAP["ENTER"])
    
    # 3. Wait for user interaction
    print("  Waiting for window to close (Manual or Auto)...")
    for i in range(10):
        time.sleep(1.0)
        if check_closed():
            print("  Window closed! Success.")
            return
        print("  Window still open...")

    print("WARNING: Window remained open.")
    print("Done.")

if __name__ == "__main__":
    main()
