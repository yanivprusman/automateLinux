import socket
import json
import subprocess
import os
import threading
import signal
import sys

PORT = 3505
BASE_DIR = "/opt/automateLinux"

def log(msg):
    print(f"[Manager] {msg}")
    sys.stdout.flush()

def run_command(cmd, cwd=BASE_DIR):
    log(f"Executing: {cmd} in {cwd}")
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, timeout=300)
        return result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return 124, "Command timed out after 5 minutes"
    except Exception as e:
        return 1, str(e)

def handle_client(conn, addr):
    log(f"Connection from {addr}")
    try:
        data = conn.recv(4096).decode('utf-8')
        if not data:
            return
        
        try:
            request = json.loads(data)
        except json.JSONDecodeError:
            conn.sendall(b"Invalid JSON\n")
            return

        command = request.get("command")
        log(f"Received command: {command}")

        if command == "deploy":
            # 1. Pull
            code, out1 = run_command("git pull")
            if code != 0:
                conn.sendall(json.dumps({"status": code, "output": f"Pull failed:\n{out1}"}).encode() + b"\n")
                return
            
            # 2. Build and Restart
            # We run build.sh. We use SKIP_SERVICE_RESTART=1 if we want to handle restart here, 
            # but build.sh already handles it. To ensure we return BEFORE the restart kills the daemon 
            # (if we were called BY the daemon), we should have returned already.
            # But wait, the Manager is UNRELATED to the daemon. So Manager can wait for build.sh to finish.
            code, out2 = run_command("bash -c 'cd daemon && source ./build.sh'")
            
            conn.sendall(json.dumps({"status": code, "output": f"Pull:\n{out1}\n\nBuild:\n{out2}"}).encode() + b"\n")

        elif command == "restart-daemon":
            code, out = run_command("sudo systemctl restart daemon.service")
            conn.sendall(json.dumps({"status": code, "output": out}).encode() + b"\n")

        elif command == "status":
            code1, out1 = run_command("systemctl is-active daemon.service")
            code2, out2 = run_command("git rev-list --count HEAD")
            conn.sendall(json.dumps({
                "daemon_active": out1.strip() == "active",
                "version": out2.strip(),
                "status": 0
            }).encode() + b"\n")
        
        else:
            conn.sendall(json.dumps({"status": 1, "error": "Unknown command"}).encode() + b"\n")

    except Exception as e:
        log(f"Error handling client: {e}")
    finally:
        conn.close()

def start_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind(('0.0.0.0', PORT))
        sock.listen(5)
        log(f"Manager listening on port {PORT}")
    except Exception as e:
        log(f"Failed to bind to port {PORT}: {e}")
        return

    while True:
        conn, addr = sock.accept()
        thread = threading.Thread(target=handle_client, args=(conn, addr))
        thread.start()

if __name__ == "__main__":
    start_server()
