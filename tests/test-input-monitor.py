#!/usr/bin/env python3
import evdev
import sys
import time
import os

def find_virtual_device():
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        if "AutomateLinux Virtual Device" in device.name:
            return device
    return None

def main():
    print("--- AutomateLinux Input Propagation Diagnostic ---")
    
    if os.geteuid() != 0:
        print("ERROR: This script must be run as root (sudo) to access input devices.")
        sys.exit(1)

    vdev = find_virtual_device()
    if not vdev:
        print("ERROR: 'AutomateLinux Virtual Device' not found.")
        print("Ensure the daemon is running (sudo systemctl status daemon).")
        sys.exit(1)

    print(f"Targeting: {vdev.name} ({vdev.path})")
    print(f"Metadata: phys={vdev.phys} uniq={vdev.uniq}")
    
    print("\n[STEP 1] Checking Capabilities...")
    caps = vdev.capabilities()
    has_keys = evdev.ecodes.EV_KEY in caps
    has_rel = evdev.ecodes.EV_REL in caps
    has_syn = evdev.ecodes.EV_SYN in caps
    
    print(f"- Has Keyboard/Buttons (EV_KEY): {has_keys}")
    print(f"- Has Relative Motion (EV_REL): {has_rel}")
    print(f"- Has Sync Events (EV_SYN): {has_syn}")
    
    if not has_keys:
        print("WARNING: Virtual device missing KEY/BUTTON capabilities!")
    
    print("\n[STEP 2] Live Monitor")
    print("Please perform the following actions:")
    print("1. Press ENTER then wait 2 seconds.")
    print("2. Left Click then wait 2 seconds.")
    print("3. Press 'x' then wait 2 seconds.")
    print("\nMonitoring (Press Ctrl+C to stop)...\n")

    try:
        start_time = time.time()
        for event in vdev.read_loop():
            # Filter for KEY/BUTTON events
            if event.type == evdev.ecodes.EV_KEY:
                key_name = evdev.ecodes.KEY.get(event.code) or evdev.ecodes.BTN.get(event.code) or "UNKNOWN"
                state = "PRESS" if event.value == 1 else "RELEASE" if event.value == 0 else "REPEAT"
                print(f"[VDEV EVENT] Time: {event.timestamp():.3f} | Code: {event.code} ({key_name}) | Value: {event.value} ({state})")
            
            # Explicitly log SYNs because they matter for propagation
            elif event.type == evdev.ecodes.EV_SYN:
                 print(f"[VDEV EVENT] Time: {event.timestamp():.3f} | --- EV_SYN (SYN_REPORT) ---")

    except KeyboardInterrupt:
        print("\nStopping monitor.")

if __name__ == "__main__":
    main()
