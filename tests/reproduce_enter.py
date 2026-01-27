#!/usr/bin/env python3
import evdev
import threading
import subprocess
import time
import sys
import os

KEYWORDS = ["AutomateLinux Virtual Device"]
TARGET_KEY = evdev.ecodes.KEY_ENTER

received_events = []

def find_virtual_device():
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        for keyword in KEYWORDS:
            if keyword in device.name:
                return device
    return None

def monitor_device(device):
    print(f"Monitoring {device.name}...")
    try:
        for event in device.read_loop():
            if event.type == evdev.ecodes.EV_KEY:
                if event.code == TARGET_KEY:
                    state = "PRESS" if event.value == 1 else "RELEASE" if event.value == 0 else "REPEAT"
                    print(f"Captured: ENTER ({state})")
                    received_events.append((event.code, event.value))
    except (OSError, Exception) as e:
        print(f"Monitor stopped: {e}")

def get_keyboard_path():
    try:
        with open("/opt/automateLinux/data/keyboardPath.txt", "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        print("Error: keyboardPath.txt not found")
        return None

def inject_enter(device_path):
    print(f"Injecting ENTER to {device_path}...")
    try:
        # Press
        subprocess.run(["evemu-event", device_path, "--type", "EV_KEY", "--code", "KEY_ENTER", "--value", "1"], check=True)
        time.sleep(0.05)
        subprocess.run(["evemu-event", device_path, "--type", "EV_SYN", "--code", "SYN_REPORT", "--value", "0"], check=True) # Sync
        time.sleep(0.1)
        # Release
        subprocess.run(["evemu-event", device_path, "--type", "EV_KEY", "--code", "KEY_ENTER", "--value", "0"], check=True)
        time.sleep(0.05)
        subprocess.run(["evemu-event", device_path, "--type", "EV_SYN", "--code", "SYN_REPORT", "--value", "0"], check=True) # Sync
        print("Injection complete.")
    except Exception as e:
        print(f"Injection failed: {e}")

def main():
    if os.geteuid() != 0:
        print("Warning: Not running as root. This might fail.")

    vdev = find_virtual_device()
    if not vdev:
        print("Virtual device not found! Is daemon running?")
        sys.exit(1)

    # Start monitor thread
    t = threading.Thread(target=monitor_device, args=(vdev,), daemon=True)
    t.start()

    kbd_path = get_keyboard_path()
    if not kbd_path:
        sys.exit(1)

    time.sleep(1) # Wait for monitor
    inject_enter(kbd_path)
    time.sleep(1) # Wait for propagation

    # analyze
    presses = [e for e in received_events if e[1] == 1]
    releases = [e for e in received_events if e[1] == 0]

    print("-" * 30)
    if presses and releases:
        print("SUCCESS: ENTER key propagated through daemon!")
        sys.exit(0)
    else:
        print("FAILURE: ENTER key NOT detected on virtual output.")
        sys.exit(1)

if __name__ == "__main__":
    main()
