import socket
import json
import subprocess
import os
import threading
import signal
import sys

PORT = 3505
BASE_DIR = "/opt/automateLinux"
UDS_PATH = "/run/automatelinux/automatelinux-daemon.sock"
SERVICES_DIR = os.path.join(BASE_DIR, "services/system")
GENERATED_DIR = os.path.join(SERVICES_DIR, "generated")
SYSTEM_DIR = "/etc/systemd/system"

def log(msg):
    print(f"[Manager] {msg}")
    sys.stdout.flush()

def run_command(cmd, cwd=BASE_DIR, timeout=300):
    log(f"Executing: {cmd} in {cwd}")
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, timeout=timeout)
        return result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return 124, f"Command timed out after {timeout}s"
    except Exception as e:
        return 1, str(e)

def send_to_daemon(command_obj):
    """Send a command to the local daemon via Unix socket."""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(UDS_PATH)
        msg = json.dumps(command_obj) + "\n"
        sock.sendall(msg.encode())
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
            if response.endswith(b"\n"):
                break
        sock.close()
        return response.decode().strip()
    except Exception as e:
        log(f"Failed to send to daemon: {e}")
        return None

def fix_permissions(path):
    """Fix ownership and permissions for automateLinux directories."""
    if not os.path.exists(path):
        return
    run_command(f"chown -R root:coding {path}")
    run_command(f"chmod -R g+w {path}")
    run_command(f"find {path} -type d -exec chmod g+s {{}} +")

def resolve_service_name(template, app_id, mode):
    """Resolve service name template like '{app}-{mode}' -> 'cad-dev'."""
    return template.replace("{app}", app_id).replace("{mode}", mode)

def install_service_files(request):
    """Install systemd service files for an app (hybrid: hand-written or generated)."""
    app_id = request["app_id"]
    display_name = request.get("display_name", app_id)
    client_template = request.get("client_service_template", "")
    server_template = request.get("server_service_template", "")
    has_server = request.get("has_server", False)
    client_subdir = request.get("client_subdir", "")
    dev_path = request.get("dev_path", f"/opt/dev/{app_id}")
    prod_path = request.get("prod_path", f"/opt/prod/{app_id}")
    ports = request.get("ports", {})

    os.makedirs(GENERATED_DIR, exist_ok=True)

    results = []
    service_names = []

    # Collect all services to install
    if client_template:
        service_names.append(("client", "dev", resolve_service_name(client_template, app_id, "dev")))
        if prod_path:
            service_names.append(("client", "prod", resolve_service_name(client_template, app_id, "prod")))
    if has_server and server_template:
        service_names.append(("server", "dev", resolve_service_name(server_template, app_id, "dev")))
        if prod_path:
            service_names.append(("server", "prod", resolve_service_name(server_template, app_id, "prod")))

    for component, mode, svc_name in service_names:
        svc_file = f"{svc_name}.service"
        hand_written = os.path.join(SERVICES_DIR, svc_file)
        generated = os.path.join(GENERATED_DIR, svc_file)
        system_link = os.path.join(SYSTEM_DIR, svc_file)

        if os.path.isfile(hand_written):
            # Symlink hand-written
            os.symlink(hand_written, system_link) if not os.path.exists(system_link) else os.replace(hand_written, system_link) or None
            run_command(f"ln -sf {hand_written} {system_link}")
            results.append(f"{svc_file} -> linked (hand-written)")
        else:
            # Generate service file
            app_path = dev_path if mode == "dev" else prod_path
            work_dir = f"{app_path}/{client_subdir}" if client_subdir else app_path
            npm_script = "dev" if mode == "dev" else "start"
            node_env = "development" if mode == "dev" else "production"

            # Find port from ports dict
            port_key = f"{app_id}-{mode}" if component == "client" else f"{app_id}-server"
            port = ports.get(port_key)
            port_arg = f" -- -p {port}" if port else ""

            content = f"""[Unit]
Description={display_name} ({mode})
After=network.target daemon.service
Wants=daemon.service

[Service]
Type=simple
WorkingDirectory={work_dir}
ExecStart=/usr/bin/npm run {npm_script}{port_arg}
Restart=on-failure
RestartSec=5s
User=root
Group=root
Environment=NODE_ENV={node_env}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
"""
            with open(generated, "w") as f:
                f.write(content)
            run_command(f"ln -sf {generated} {system_link}")
            results.append(f"{svc_file} -> linked (generated)")

    run_command("systemctl daemon-reload")
    results.append("systemctl daemon-reload: OK")
    return results

def handle_install_app(request):
    """Handle install-app command: clone, worktree, deps, build, services, ports, perms."""
    app_id = request["app_id"]
    repo_url = request.get("repo_url", "")
    dev_path = request.get("dev_path", f"/opt/dev/{app_id}")
    prod_path = request.get("prod_path", f"/opt/prod/{app_id}")
    has_server = request.get("has_server", False)
    server_build_subdir = request.get("server_build_subdir", "")
    client_subdir = request.get("client_subdir", "")
    ports = request.get("ports", {})
    mode = request.get("mode", "all")

    steps = []

    # 1. Clone or pull
    if os.path.isdir(dev_path):
        code, out = run_command(f"git -C {dev_path} pull")
        steps.append({"step": "git pull", "status": code, "output": out.strip()})
    else:
        os.makedirs(os.path.dirname(dev_path), exist_ok=True)
        code, out = run_command(f"git clone {repo_url} {dev_path}")
        if code != 0:
            return {"status": 1, "output": f"Clone failed:\n{out}", "steps": steps}
        steps.append({"step": "git clone", "status": code, "output": out.strip()})

    # 2. Create prod worktree
    if prod_path and not os.path.isdir(prod_path):
        os.makedirs(os.path.dirname(prod_path), exist_ok=True)
        code, commit = run_command(f"git -C {dev_path} rev-parse HEAD")
        commit = commit.strip()
        code, out = run_command(f"git -C {dev_path} worktree add --detach {prod_path} {commit}")
        steps.append({"step": "worktree", "status": code, "output": out.strip()})
    else:
        steps.append({"step": "worktree", "status": 0, "output": "already exists"})

    # 3. npm install
    install_paths = []
    if mode in ("all", "dev"):
        if client_subdir:
            install_paths.append(f"{dev_path}/{client_subdir}")
        else:
            install_paths.append(dev_path)
    if mode in ("all", "prod") and prod_path:
        if client_subdir:
            install_paths.append(f"{prod_path}/{client_subdir}")
        else:
            install_paths.append(prod_path)

    for p in install_paths:
        pkg_json = os.path.join(p, "package.json")
        if os.path.isfile(pkg_json):
            code, out = run_command(f"npm install", cwd=p, timeout=120)
            steps.append({"step": f"npm install ({p})", "status": code, "output": out.strip()[-200:]})

    # 4. C++ build
    if has_server and server_build_subdir:
        build_paths = []
        if mode in ("all", "dev"):
            build_paths.append(f"{dev_path}/{server_build_subdir}")
        if mode in ("all", "prod") and prod_path:
            build_paths.append(f"{prod_path}/{server_build_subdir}")

        for bp in build_paths:
            cmake_file = os.path.join(bp, "CMakeLists.txt")
            if os.path.isfile(cmake_file):
                build_dir = os.path.join(bp, "build")
                os.makedirs(build_dir, exist_ok=True)
                code, out = run_command(f"cd {build_dir} && cmake .. && make -j$(nproc)", timeout=120)
                steps.append({"step": f"C++ build ({bp})", "status": code, "output": out.strip()[-200:]})

    # 5. Install service files
    svc_results = install_service_files(request)
    steps.append({"step": "services", "status": 0, "output": "; ".join(svc_results)})

    # 6. Set ports on local daemon
    for port_key, port_val in ports.items():
        resp = send_to_daemon({"command": "setPort", "key": port_key, "value": str(port_val)})
        steps.append({"step": f"setPort {port_key}={port_val}", "status": 0 if resp else 1, "output": resp or "failed"})

    # 7. Fix permissions
    fix_permissions(dev_path)
    if prod_path:
        fix_permissions(prod_path)
    steps.append({"step": "permissions", "status": 0, "output": "OK"})

    # Determine overall status
    failed = [s for s in steps if s["status"] != 0]
    overall = 1 if failed else 0

    # Build readable output
    output_lines = [f"Install {app_id}:"]
    for s in steps:
        status_str = "OK" if s["status"] == 0 else f"FAIL({s['status']})"
        output_lines.append(f"  {s['step']}: {status_str}")
        if s["status"] != 0 and s["output"]:
            output_lines.append(f"    {s['output']}")

    return {"status": overall, "output": "\n".join(output_lines) + "\n"}

def handle_uninstall_app(request):
    """Handle uninstall-app: stop services, remove files, clean up."""
    app_id = request["app_id"]
    dev_path = request.get("dev_path", f"/opt/dev/{app_id}")
    prod_path = request.get("prod_path", f"/opt/prod/{app_id}")
    client_template = request.get("client_service_template", "")
    server_template = request.get("server_service_template", "")
    has_server = request.get("has_server", False)
    port_key_client = request.get("port_key_client", app_id)
    port_key_server = request.get("port_key_server", "")

    steps = []

    # 1. Stop and disable all services
    service_names = []
    if client_template:
        service_names.append(resolve_service_name(client_template, app_id, "dev"))
        service_names.append(resolve_service_name(client_template, app_id, "prod"))
    if has_server and server_template:
        service_names.append(resolve_service_name(server_template, app_id, "dev"))
        service_names.append(resolve_service_name(server_template, app_id, "prod"))

    for svc in service_names:
        run_command(f"systemctl stop {svc} 2>/dev/null")
        run_command(f"systemctl disable {svc} 2>/dev/null")
        # Remove service file links and generated files
        for d in [SYSTEM_DIR, GENERATED_DIR]:
            svc_file = os.path.join(d, f"{svc}.service")
            if os.path.exists(svc_file):
                os.remove(svc_file)
    steps.append({"step": "stop+disable services", "status": 0, "output": ", ".join(service_names)})

    # 2. Remove prod worktree
    if prod_path and os.path.isdir(prod_path):
        code, out = run_command(f"git -C {dev_path} worktree remove {prod_path} --force 2>/dev/null; rm -rf {prod_path}")
        steps.append({"step": "remove prod worktree", "status": 0, "output": out.strip()})

    # 3. Remove dev directory
    if os.path.isdir(dev_path):
        code, out = run_command(f"rm -rf {dev_path}")
        steps.append({"step": "remove dev directory", "status": code, "output": out.strip()})

    # 4. Delete ports from daemon
    for key in [f"{port_key_client}-prod", f"{port_key_client}-dev"]:
        send_to_daemon({"command": "deletePort", "key": key})
    if has_server and port_key_server:
        send_to_daemon({"command": "deletePort", "key": port_key_server})
    steps.append({"step": "delete ports", "status": 0, "output": "OK"})

    # 5. Remove from extra apps registry
    send_to_daemon({"command": "removeExtraApp", "app": app_id})
    steps.append({"step": "remove from registry", "status": 0, "output": "OK"})

    # 6. Reload systemd
    run_command("systemctl daemon-reload")
    steps.append({"step": "daemon-reload", "status": 0, "output": "OK"})

    output_lines = [f"Uninstall {app_id}:"]
    for s in steps:
        status_str = "OK" if s["status"] == 0 else f"FAIL({s['status']})"
        output_lines.append(f"  {s['step']}: {status_str}")

    return {"status": 0, "output": "\n".join(output_lines) + "\n"}

def handle_app_control(request):
    """Handle app-control: relay start/stop to local daemon via Unix socket."""
    action = request.get("action")
    app_id = request.get("app_id")
    mode = request.get("mode", "dev")

    if action == "start":
        resp = send_to_daemon({"command": "startApp", "app": app_id, "mode": mode})
    elif action == "stop":
        resp = send_to_daemon({"command": "stopApp", "app": app_id, "mode": mode})
    else:
        return {"status": 1, "output": f"Unknown action: {action}\n"}

    if resp is None:
        return {"status": 1, "output": f"Failed to communicate with local daemon\n"}

    return {"status": 0, "output": resp}

def handle_client(conn, addr):
    log(f"Connection from {addr}")
    try:
        # Read potentially large messages
        data = b""
        while True:
            chunk = conn.recv(8192)
            if not chunk:
                break
            data += chunk
            if data.endswith(b"\n"):
                break
        data = data.decode('utf-8')
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

        elif command == "install-app":
            result = handle_install_app(request)
            conn.sendall(json.dumps(result).encode() + b"\n")

        elif command == "uninstall-app":
            result = handle_uninstall_app(request)
            conn.sendall(json.dumps(result).encode() + b"\n")

        elif command == "app-control":
            result = handle_app_control(request)
            conn.sendall(json.dumps(result).encode() + b"\n")

        else:
            conn.sendall(json.dumps({"status": 1, "error": "Unknown command"}).encode() + b"\n")

    except Exception as e:
        log(f"Error handling client: {e}")
        try:
            conn.sendall(json.dumps({"status": 1, "output": f"Manager error: {str(e)}\n"}).encode() + b"\n")
        except:
            pass
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
