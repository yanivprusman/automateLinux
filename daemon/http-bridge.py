#!/usr/bin/python3
import http.server
import socketserver
import json
import socket
import os
import sys
import threading
import queue
import time

PORT = 9223
SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

# Global list of SSE clients (wfile objects)
sse_clients = []
sse_lock = threading.Lock()

def debug_log(msg):
    try:
        with open("/home/yaniv/coding/automateLinux/data/combined.log", "a") as f:
            f.write(f"[Bridge] {msg}\n")
    except:
        pass

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/active-tab':
            try:
                content_length = int(self.headers['Content-Length'])
                post_data = self.rfile.read(content_length)
                debug_log(f"POST /active-tab received: {post_data.decode('utf-8')}")
                
                data = json.loads(post_data)
                url = data.get('url', '')
                
                # Send to daemon socket (new connection for each request to avoid blocking)
                if url:
                    DaemonLink.send_one_off({"command": "setActiveTabUrl", "url": url})
                    response = {"status": "ok"}
                else:
                    response = {"status": "error", "message": "No URL provided"}
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                self.wfile.write(json.dumps(response).encode('utf-8'))
                
            except Exception as e:
                debug_log(f"POST /active-tab error: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(str(e).encode('utf-8'))

    def do_GET(self):
        if self.path == '/events':
            # Server-Sent Events endpoint
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            # Register client
            q = queue.Queue()
            with sse_lock:
                sse_clients.append(q)
            debug_log("New SSE client connected")
            
            try:
                while True:
                    msg = q.get() # Blocks until message available
                    debug_log(f"Sending SSE: {msg}")
                    self.wfile.write(f"data: {json.dumps(msg)}\n\n".encode('utf-8'))
                    self.wfile.flush()
            except Exception as e:
                debug_log(f"SSE client disconnected: {e}")
            finally:
                with sse_lock:
                    if q in sse_clients:
                        sse_clients.remove(q)
        else:
            self.send_response(404)
            self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def log_message(self, format, *args):
        pass

class DaemonLink:
    @staticmethod
    def send_one_off(message):
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.connect(SOCKET_PATH)
                cmd = json.dumps(message) + "\n"
                s.sendall(cmd.encode('utf-8'))
        except Exception as e:
            pass

    def __init__(self):
        self.sock = None
        self.running = True

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.connect(SOCKET_PATH)
            # Register to receive messages
            register_cmd = json.dumps({"command": "registerNativeHost"}) + "\n"
            self.sock.sendall(register_cmd.encode('utf-8'))
            debug_log("Connected to daemon socket")
            return True
        except Exception as e:
            # debug_log(f"Connect failed: {e}")
            return False

    def listen_loop(self):
        buffer = ""
        while self.running:
            if not self.sock:
                if not self.connect():
                    time.sleep(1)
                    continue
            
            try:
                data = self.sock.recv(4096).decode('utf-8')
                if not data:
                    self.sock = None
                    debug_log("Daemon disconnected (recv returned empty)")
                    continue
                
                debug_log(f"Received {len(data)} bytes: {repr(data[:100])}")
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    if not line.strip():
                        continue
                    try:
                        msg = json.loads(line)
                        # Broadcast to SSE clients
                        debug_log(f"Broadcasting: {msg}")
                        with sse_lock:
                            for q in sse_clients:
                                q.put(msg)
                    except Exception as e:
                        debug_log(f"Error handling msg: {e}")
            except Exception as e:
                debug_log(f"Socket error: {e}")
                self.sock = None
                time.sleep(1)

def main():
    # Start daemon listener thread
    link = DaemonLink()
    t = threading.Thread(target=link.listen_loop, daemon=True)
    t.start()
    
    # Start HTTP server
    if not os.path.exists(SOCKET_PATH):
        pass # Wait for daemon
    
    with socketserver.ThreadingTCPServer(("127.0.0.1", PORT), Handler) as httpd:
        httpd.daemon_threads = True
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    main()
