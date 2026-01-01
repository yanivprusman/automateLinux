#!/usr/bin/env python3
import evdev
import json
import socket
import sys
import threading
import time
import os

SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

def find_virtual_device():
    # Wait a bit for device to settle if just restarted
    time.sleep(1)
    for path in evdev.list_devices():
        try:
            device = evdev.InputDevice(path)
            if "AutomateLinux Virtual Device" in device.name:
                return device
        except:
            continue
    return None

def send_command(cmd_dict):
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.connect(SOCKET_PATH)
            s.sendall(json.dumps(cmd_dict).encode() + b"\n")
            response = s.recv(4096)
            return response.decode()
    except Exception as e:
        return f"Error: {e}"

def monitor_device(vdev, expected_codes, results):
    # expected_codes is a dict: {code: name}
    print(f"[*] Monitoring {vdev.path}...")
    try:
        vdev.grab()
        start_time = time.time()
        while time.time() - start_time < 3: # 3 second timeout
            event = vdev.read_one()
            if event:
                if event.type == evdev.ecodes.EV_KEY and event.code in expected_codes:
                    if event.value == 1: # Only care about press
                         print(f"[+] SEEN: {expected_codes[event.code]}")
                         results.add(event.code)
            else:
                time.sleep(0.01)
            
            if len(results) >= len(expected_codes):
                break
        vdev.ungrab()
    except Exception as e:
        print(f"[-] Monitor error: {e}")

def run_test(name, type_code, key_code):
    print(f"\n>>> TESTING: {name} (Code {key_code})")
    vdev = find_virtual_device()
    if not vdev:
        print("[-] FAILED: Virtual device not found")
        return False

    results = []
    # Start monitor in a thread
    t = threading.Thread(target=monitor_device, args=(vdev, key_code, results))
    t.start()
    
    time.sleep(0.5) # Let thread settle
    
    # Send simulation command (Press)
    print(f"[*] Sending simulateInput (PRESS, {key_code})...")
    send_command({"command": "simulateInput", "type": type_code, "code": key_code, "value": 1})
    time.sleep(0.2)
    
    # Send simulation command (Release)
    print(f"[*] Sending simulateInput (RELEASE, {key_code})...")
    send_command({"command": "simulateInput", "type": type_code, "code": key_code, "value": 0})
    
    t.join()
    
    if len(results) >= 2:
        print(f"[SUCCESS] {name} propagated correctly to virtual device.")
        return True
    else:
        print(f"[FAILURE] {name} was NOT detected on virtual device.")
        return False

def main():
    print("--- AutomateLinux Loopback Diagnostic ---")
    
    vdev = find_virtual_device()
    if not vdev:
        print("[-] FAILED: Virtual device not found")
        sys.exit(1)

    # Codes to look for
    KEY_ENTER = 28
    BTN_LEFT = 272
    targets = {KEY_ENTER: "ENTER", BTN_LEFT: "CLICK"}
    seen = set()

    # Signal to daemon that we are listening
    print("READY") 
    sys.stdout.flush()

    monitor_device(vdev, targets, seen)
    
    e_ok = KEY_ENTER in seen
    m_ok = BTN_LEFT in seen
    
    print("\n--- Summary ---")
    print(f"Keyboard Pipeline: {'OK' if e_ok else 'BROKEN'}")
    print(f"Mouse Pipeline:    {'OK' if m_ok else 'BROKEN'}")
    
    if e_ok and m_ok:
        print("\n[CONCLUSION] The daemon-to-kernel pipeline is working perfectly.")
    else:
        print("\n[CONCLUSION] Input failure detected.")

if __name__ == "__main__":
    main()
