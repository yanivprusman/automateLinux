catWireGuard() {
    local SERVER_USER="root"
    local SERVER_IP="31.133.102.195"
    
    echo "=========================================="
    echo "  WIREGUARD & NGINX CONFIGURATION DUMP"
    echo "=========================================="
    echo
    
    echo "===== LOCAL PC FILES ====="
    echo
    
    echo "--- /etc/wireguard/wg0.conf ---"
    if sudo cat /etc/wireguard/wg0.conf 2>/dev/null; then
        echo
    else
        echo "[File not found or no permission]"
        echo
    fi
    
    echo "--- /etc/nginx/sites-available/default ---"
    if sudo cat /etc/nginx/sites-available/default 2>/dev/null; then
        echo
    else
        echo "[File not found or Nginx not installed]"
        echo
    fi
    
    echo "--- /etc/nginx/sites-available/pc-proxy ---"
    if sudo cat /etc/nginx/sites-available/pc-proxy 2>/dev/null; then
        echo
    else
        echo "[File not found]"
        echo
    fi
    
    echo "--- /etc/nginx/sites-enabled/ (symlinks) ---"
    if sudo ls -la /etc/nginx/sites-enabled/ 2>/dev/null; then
        echo
    else
        echo "[Directory not found]"
        echo
    fi
    
    echo "=========================================="
    echo "===== SERVER ($SERVER_IP) FILES ====="
    echo "=========================================="
    echo
    
    if ssh -o BatchMode=yes -o ConnectTimeout=10 "$SERVER_USER@$SERVER_IP" bash << 'EOF'
echo "--- /etc/wireguard/wg0.conf ---"
if cat /etc/wireguard/wg0.conf 2>/dev/null; then
    echo
else
    echo "[File not found or no permission]"
    echo
fi

echo "--- /etc/nginx/sites-available/default ---"
if cat /etc/nginx/sites-available/default 2>/dev/null; then
    echo
else
    echo "[File not found or Nginx not installed]"
    echo
fi

echo "--- /etc/nginx/sites-available/pc-proxy ---"
if cat /etc/nginx/sites-available/pc-proxy 2>/dev/null; then
    echo
else
    echo "[File not found]"
    echo
fi

echo "--- /etc/nginx/sites-enabled/ (symlinks) ---"
if ls -la /etc/nginx/sites-enabled/ 2>/dev/null; then
    echo
else
    echo "[Directory not found]"
    echo
fi

echo "--- Nginx status ---"
systemctl status nginx --no-pager -l 2>/dev/null || echo "[Nginx not running or not installed]"
echo

echo "--- WireGuard status ---"
wg show 2>/dev/null || echo "[WireGuard not active]"
echo
EOF
    then
        echo
        echo "=========================================="
        echo "  DUMP COMPLETE"
        echo "=========================================="
    else
        echo
        echo "[ERROR: Could not connect to server]"
        echo "Check SSH connection or add your key with: ssh-copy-id $SERVER_USER@$SERVER_IP"
    fi
}

setupWireGuardProxy() {
    local SERVER_USER="root"
    local SERVER_IP="31.133.102.195"
    local TEST_DIR="$HOME/wireguard-test-site"
    local SERVER_PORT="8080"
    local PC_PORT="80"
    
    echo "=========================================="
    echo "  WIREGUARD PROXY SETUP"
    echo "=========================================="
    echo
    
    # Step 1: Create test site on PC
    echo "📁 Creating test site on PC..."
    mkdir -p "$TEST_DIR"
    cat > "$TEST_DIR/index.html" << 'HTML'
<!DOCTYPE html>
<html>
<head>
    <title>WireGuard Test Site</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .container {
            background: rgba(255,255,255,0.1);
            padding: 30px;
            border-radius: 10px;
            backdrop-filter: blur(10px);
        }
        h1 { margin-top: 0; }
        .success { color: #00ff00; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎉 WireGuard Proxy Working!</h1>
        <p class="success">✓ Connection successful through WireGuard tunnel</p>
        <p>This page is being served from your PC at <code>10.0.0.2</code></p>
        <p>You're accessing it via your Kamatera server at <code>31.133.102.195:8080</code></p>
        <hr>
        <p><strong>Server time:</strong> <span id="time"></span></p>
        <script>
            setInterval(() => {
                document.getElementById('time').textContent = new Date().toLocaleString();
            }, 1000);
        </script>
    </div>
</body>
</html>
HTML
    echo "✓ Test site created at $TEST_DIR"
    echo
    
    # Step 2: Check if Nginx is installed on PC
    echo "🔍 Checking for Nginx on PC..."
    if command -v nginx &> /dev/null; then
        echo "✓ Nginx found on PC"
        
        # Configure Nginx on PC
        echo "⚙️  Configuring Nginx on PC..."
        sudo tee /etc/nginx/sites-available/wireguard-test > /dev/null << NGINX
server {
    listen $PC_PORT;
    listen [::]:$PC_PORT;
    
    root $TEST_DIR;
    index index.html;
    
    server_name _;
    
    location / {
        try_files \$uri \$uri/ =404;
    }
}
NGINX
        
        # Enable the site
        sudo ln -sf /etc/nginx/sites-available/wireguard-test /etc/nginx/sites-enabled/wireguard-test
        
        # Test and reload
        if sudo nginx -t 2>&1; then
            sudo systemctl reload nginx
            echo "✓ Nginx configured and reloaded"
            PC_WEBSERVER="nginx"
        else
            echo "⚠️  Nginx configuration error, falling back to Python"
            PC_WEBSERVER="python"
        fi
    else
        echo "⚠️  Nginx not found, will use Python HTTP server"
        PC_WEBSERVER="python"
    fi
    echo
    
    # Step 3: Setup server-side proxy
    echo "🌐 Setting up reverse proxy on server..."
    
    if ssh "$SERVER_USER@$SERVER_IP" bash << EOF
# Create pc-proxy config
cat > /etc/nginx/sites-available/pc-proxy << 'PROXY'
server {
    listen $SERVER_PORT;
    listen [::]:$SERVER_PORT;
    
    location / {
        proxy_pass http://10.0.0.2:$PC_PORT;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        
        # Timeout settings
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }
}
PROXY

# Enable the site
ln -sf /etc/nginx/sites-available/pc-proxy /etc/nginx/sites-enabled/pc-proxy

# Test and reload
if nginx -t 2>&1; then
    systemctl reload nginx
    echo "✓ Server Nginx configured and reloaded"
    exit 0
else
    echo "✗ Server Nginx configuration error"
    exit 1
fi
EOF
    then
        echo "✓ Server configuration complete"
    else
        echo "✗ Failed to configure server"
        return 1
    fi
    echo
    
    # Step 4: Start Python server if needed
    if [ "$PC_WEBSERVER" = "python" ]; then
        echo "🐍 Starting Python HTTP server on port $PC_PORT..."
        echo "   (Press Ctrl+C to stop when done testing)"
        echo
        cd "$TEST_DIR"
        sudo python3 -m http.server $PC_PORT
    fi
    
    # Step 5: Test the connection
    echo "=========================================="
    echo "  SETUP COMPLETE!"
    echo "=========================================="
    echo
    echo "📍 Access your PC from anywhere at:"
    echo "   http://$SERVER_IP:$SERVER_PORT"
    echo
    echo "🧪 Test it now:"
    echo "   curl http://$SERVER_IP:$SERVER_PORT"
    echo
    
    if [ "$PC_WEBSERVER" = "nginx" ]; then
        echo "✓ Using Nginx (will run in background)"
        echo
        echo "To stop: sudo systemctl stop nginx"
        echo "To restart: sudo systemctl restart nginx"
    else
        echo "⚠️  Python server running in foreground"
        echo "   Keep this terminal open for testing"
        echo "   Press Ctrl+C when done"
    fi
}