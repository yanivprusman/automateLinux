#!/usr/bin/python3
"""
Fully automated focus+paste test using uinput to simulate Ctrl+V.
Tests the complete flow: defocus → Ctrl+V → focus → paste
"""

import subprocess
import time
import sys
import os

LOG_FILE = "/home/yaniv/coding/automateLinux/data/combined.log"

def log(msg):
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] {msg}")

def simulate_ctrl_v():
    """Simulate Ctrl+V keypress using evemu-event."""
    try:
        # Find keyboard device
        result = subprocess.run(
            ["cat", "/home/yaniv/coding/automateLinux/data/keyboardPath.txt"],
            capture_output=True,
            text=True
        )
        kbd_path = result.stdout.strip()
        
        if not kbd_path or not os.path.exists(kbd_path):
            log(f"✗ Keyboard device not found: {kbd_path}")
            return False
        
        log(f"Using keyboard: {kbd_path}")
        
        # Simulate Ctrl+V sequence
        # Ctrl down, V down, V up, Ctrl up
        subprocess.run(["evemu-event", kbd_path, "--type", "EV_KEY", "--code", "KEY_LEFTCTRL", "--value", "1"], check=True)
        time.sleep(0.01)
        subprocess.run(["evemu-event", kbd_path, "--type", "EV_KEY", "--code", "KEY_V", "--value", "1"], check=True)
        time.sleep(0.01)
        subprocess.run(["evemu-event", kbd_path, "--type", "EV_KEY", "--code", "KEY_V", "--value", "0"], check=True)
        time.sleep(0.01)
        subprocess.run(["evemu-event", kbd_path, "--type", "EV_KEY", "--code", "KEY_LEFTCTRL", "--value", "0"], check=True)
        
        log("✓ Ctrl+V simulated")
        return True
    except Exception as e:
        log(f"✗ Failed to simulate Ctrl+V: {e}")
        return False

def check_logs(marker):
    """Check logs for focus and ACK events."""
    try:
        result = subprocess.run(
            ["tail", "-50", LOG_FILE],
            capture_output=True,
            text=True
        )
        logs = result.stdout
        
        if marker not in logs:
            log("⚠ Test marker not found in logs")
            return False
        
        # Extract logs after marker
        marker_idx = logs.find(marker)
        relevant_logs = logs[marker_idx:]
        
        focus_sent = "Sent focus request" in relevant_logs
        ack_received = "Focus ACK received" in relevant_logs
        timeout = "Focus ACK TIMEOUT" in relevant_logs
        
        return {
            "focus_sent": focus_sent,
            "ack_received": ack_received,
            "timeout": timeout,
            "logs": relevant_logs
        }
    except Exception as e:
        log(f"✗ Error checking logs: {e}")
        return None

def main():
    log("🎯 Fully Automated Focus+Paste Test")
    log("=" * 50)
    
    # Set test marker
    marker = f"AUTOMATED_TEST_{int(time.time())}"
    with open(LOG_FILE, 'a') as f:
        f.write(f"\n{marker}\n")
    log(f"✓ Test marker set: {marker}")
    
    # Wait a moment for browser to be ready
    log("Waiting for browser to be ready...")
    time.sleep(2)
    
    # Simulate Ctrl+V
    log("⌨️  Simulating Ctrl+V...")
    start_time = time.time()
    
    if not simulate_ctrl_v():
        log("❌ TEST FAILED: Could not simulate Ctrl+V")
        sys.exit(1)
    
    # Wait for focus+paste to complete
    time.sleep(1)
    
    elapsed = int((time.time() - start_time) * 1000)
    
    # Check results
    log("📊 Analyzing results...")
    results = check_logs(marker)
    
    if not results:
        log("❌ TEST FAILED: Could not analyze logs")
        sys.exit(1)
    
    print()
    log("Results:")
    log(f"  Focus sent: {'✓' if results['focus_sent'] else '✗'}")
    log(f"  ACK received: {'✓' if results['ack_received'] else '✗'}")
    log(f"  Timeout: {'✗ YES' if results['timeout'] else '✓ No'}")
    log(f"  Total time: {elapsed}ms")
    
    if results['focus_sent'] and results['ack_received'] and not results['timeout']:
        print()
        log("✅ TEST PASSED!")
        log(f"   ✓ Focus+paste completed in {elapsed}ms")
        log("   ✓ Communication working perfectly")
        sys.exit(0)
    else:
        print()
        log("❌ TEST FAILED")
        log("Recent logs:")
        print(results['logs'][-500:])
        sys.exit(1)

if __name__ == "__main__":
    main()
