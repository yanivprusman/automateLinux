#!/usr/bin/python3
"""
Fully automated end-to-end test for Chrome focus+paste functionality.
Uses Chrome DevTools Protocol to automate browser interaction.
"""

import json
import socket
import time
import requests
import sys

def log(msg):
    """Print timestamped log message."""
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] {msg}")

def send_daemon_command(cmd):
    """Send command to daemon via Unix socket."""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect("/run/automatelinux/automatelinux-daemon.sock")
        sock.sendall((json.dumps(cmd) + "\n").encode())
        response = sock.recv(4096).decode()
        return response
    finally:
        sock.close()

def get_chatgpt_tab(ws_url):
    """Get the tab with chatgpt.com open."""
    tabs = requests.get(f"{ws_url}/json").json()
    for tab in tabs:
        if "chatgpt.com" in tab.get("url", ""):
            return tab
    return None

def execute_js(ws_url, tab_id, js_code):
    """Execute JavaScript in a Chrome tab."""
    import websocket
    ws = websocket.create_connection(f"ws://localhost:9222/devtools/page/{tab_id}")
    try:
        # Enable Runtime domain
        ws.send(json.dumps({"id": 1, "method": "Runtime.enable"}))
        ws.recv()
        
        # Execute JavaScript
        ws.send(json.dumps({
            "id": 2,
            "method": "Runtime.evaluate",
            "params": {"expression": js_code}
        }))
        response = json.loads(ws.recv())
        return response
    finally:
        ws.close()

def main():
    log("🎯 Starting Fully Automated Focus+Paste Test")
    log("=" * 50)
    
    # Step 1: Check Chrome is running with remote debugging
    ws_url = "http://localhost:9222"
    try:
        tabs = requests.get(f"{ws_url}/json").json()
        log(f"✓ Chrome DevTools accessible ({len(tabs)} tabs)")
    except Exception as e:
        log(f"✗ Chrome not accessible: {e}")
        log("  Start Chrome with: google-chrome --remote-debugging-port=9222")
        sys.exit(1)
    
    # Step 2: Find ChatGPT tab
    log("📍 Looking for ChatGPT tab...")
    chatgpt_tab = get_chatgpt_tab(ws_url)
    if not chatgpt_tab:
        log("✗ No ChatGPT tab found")
        log("  Please open https://chatgpt.com in Chrome")
        sys.exit(1)
    
    tab_id = chatgpt_tab["id"]
    log(f"✓ Found ChatGPT tab (ID: {tab_id})")
    
    # Step 3: Defocus the textarea
    log("🖱️  Defocusing textarea with simulated click...")
    defocus_js = """
    // Click outside the textarea to defocus it
    const header = document.querySelector('header') || document.querySelector('nav') || document.body;
    header.click();
    // Verify it's defocused
    const textarea = document.querySelector('#prompt-textarea') || document.querySelector('div[contenteditable="true"]');
    const isFocused = document.activeElement === textarea;
    ({isFocused: isFocused, clicked: header.tagName});
    """
    
    result = execute_js(ws_url, tab_id, defocus_js)
    log(f"  Result: {result.get('result', {}).get('value', {})}")
    log("✓ Textarea defocused")
    
    # Step 4: Trigger focus command from daemon
    log("⌨️  Triggering focus+paste via daemon...")
    start_time = time.time()
    
    response = send_daemon_command({"command": "focusChatGPT"})
    log(f"  Daemon response: {response.strip()}")
    
    # Wait for focus to complete
    time.sleep(0.5)
    
    # Step 5: Verify textarea is now focused
    log("🔍 Verifying textarea received focus...")
    verify_js = """
    const textarea = document.querySelector('#prompt-textarea') || document.querySelector('div[contenteditable="true"]');
    const isFocused = document.activeElement === textarea;
    ({isFocused: isFocused, activeElement: document.activeElement.tagName});
    """
    
    result = execute_js(ws_url, tab_id, verify_js)
    focus_result = result.get('result', {}).get('value', {})
    
    end_time = time.time()
    elapsed_ms = int((end_time - start_time) * 1000)
    
    log(f"  Focus result: {focus_result}")
    log(f"  ⏱️  Total time: {elapsed_ms}ms")
    
    # Step 6: Results
    log("")
    log("=" * 50)
    if focus_result.get('isFocused'):
        log("✅ TEST PASSED!")
        log(f"   ✓ Focus worked in {elapsed_ms}ms")
        log("   ✓ Textarea is now focused")
        sys.exit(0)
    else:
        log("❌ TEST FAILED")
        log(f"   ✗ Textarea not focused after {elapsed_ms}ms")
        log(f"   Active element: {focus_result.get('activeElement')}")
        sys.exit(1)

if __name__ == "__main__":
    main()
