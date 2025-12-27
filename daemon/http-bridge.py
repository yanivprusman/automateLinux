#!/usr/bin/python3
import http.server
import socketserver
import json
import socket
import os
import sys

PORT = 9223
SOCKET_PATH = "/run/automatelinux/automatelinux-daemon.sock"

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/active-tab':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                data = json.loads(post_data)
                url = data.get('url', '')
                
                # Send to daemon socket
                if url:
                    self.send_to_daemon({"command": "setActiveTabUrl", "url": url})
                    response = {"status": "ok"}
                else:
                    response = {"status": "error", "message": "No URL provided"}
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.send_header('Access-Control-Allow-Origin', '*') # CORS
                self.end_headers()
                self.wfile.write(json.dumps(response).encode('utf-8'))
                
            except Exception as e:
                self.send_response(500)
                self.end_headers()
                self.wfile.write(str(e).encode('utf-8'))
        elif self.path == '/focus':
             # Handle focus request from daemon/other (optional, for future)
             self.send_response(200)
             self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def send_to_daemon(self, message):
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.connect(SOCKET_PATH)
                cmd = json.dumps(message) + "\n"
                s.sendall(cmd.encode('utf-8'))
        except Exception as e:
            print(f"Failed to send to daemon: {e}", file=sys.stderr)

    def log_message(self, format, *args):
        # Silence default logging to keep console clean
        pass

if __name__ == "__main__":
    # Ensure invalid socket path errors are handled gracefully
    if not os.path.exists(SOCKET_PATH):
        print(f"Warning: Socket path {SOCKET_PATH} does not exist yet.", file=sys.stderr)
    
    with socketserver.TCPServer(("127.0.0.1", PORT), Handler) as httpd:
        print(f"HTTP Bridge serving at port {PORT}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass
