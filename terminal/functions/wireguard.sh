# Complete WireGuard Proxy Setup - Run this once from your PC
setupWireGuardProxyFromScratch() {
    local SERVER_USER="root"
    local SERVER_IP="31.133.102.195"
    
    echo "=========================================="
    echo "  COMPLETE WIREGUARD PROXY SETUP"
    echo "=========================================="
    echo
    
    # STEP 1: Fix WireGuard keys to match reality
    echo "🔑 Step 1: Syncing WireGuard keys..."
    
    local SERVER_ACTUAL_KEY=$(ssh "$SERVER_USER@$SERVER_IP" "wg show wg0 public-key 2>/dev/null")
    
    if [ -z "$SERVER_ACTUAL_KEY" ]; then
        echo "   Starting WireGuard on server first..."
        ssh "$SERVER_USER@$SERVER_IP" "wg-quick up wg0 2>/dev/null"
        sleep 2
        SERVER_ACTUAL_KEY=$(ssh "$SERVER_USER@$SERVER_IP" "wg show wg0 public-key")
    fi
    
    echo "   Server's public key: $SERVER_ACTUAL_KEY"
    
    # Update PC config with server's actual key
    sudo sed -i "/^\[Peer\]/,/^$/ s|^PublicKey = .*|PublicKey = $SERVER_ACTUAL_KEY|" /etc/wireguard/wg0.conf
    
    # Restart WireGuard on PC
    sudo wg-quick down wg0 2>/dev/null
    sudo wg-quick up wg0
    
    echo "   ✓ Keys synced"
    echo
    
    # STEP 2: Create and serve test page
    echo "📄 Step 2: Setting up web server on PC..."
    
    mkdir -p ~/wireguard-test-site
    cat > ~/wireguard-test-site/index.html << 'HTML'
<!DOCTYPE html>
<html>
<head>
    <title>WireGuard Working!</title>
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
        .success { color: #00ff00; font-weight: bold; font-size: 1.2em; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎉 Success!</h1>
        <p class="success">✓ WireGuard proxy is working!</p>
        <p>This page is served from your PC (10.0.0.2)</p>
        <p>Via your server (31.133.102.195:8080)</p>
        <p><strong>Time:</strong> <span id="time"></span></p>
        <script>
            setInterval(() => {
                document.getElementById('time').textContent = new Date().toLocaleString();
            }, 1000);
        </script>
    </div>
</body>
</html>
HTML
    
    # Configure Nginx on PC
    sudo tee /etc/nginx/sites-available/wireguard-test > /dev/null << 'NGINX'
server {
    listen 80;
    listen [::]:80;
    root /home/$USER/wireguard-test-site;
    index index.html;
    server_name _;
    location / {
        try_files $uri $uri/ =404;
    }
}
NGINX
    
    sudo sed -i "s|\$USER|$USER|g" /etc/nginx/sites-available/wireguard-test
    sudo ln -sf /etc/nginx/sites-available/wireguard-test /etc/nginx/sites-enabled/wireguard-test
    sudo nginx -t && sudo systemctl reload nginx
    
    echo "   ✓ Web server configured"
    echo
    
    # STEP 3: Setup reverse proxy on server
    echo "🌐 Step 3: Setting up reverse proxy on server..."
    
    ssh "$SERVER_USER@$SERVER_IP" bash << 'EOF'
cat > /etc/nginx/sites-available/pc-proxy << 'PROXY'
server {
    listen 8080;
    listen [::]:8080;
    
    location / {
        proxy_pass http://10.0.0.2:80;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_connect_timeout 10s;
        proxy_send_timeout 10s;
        proxy_read_timeout 10s;
    }
}
PROXY

ln -sf /etc/nginx/sites-available/pc-proxy /etc/nginx/sites-enabled/pc-proxy
nginx -t && systemctl reload nginx
EOF
    
    echo "   ✓ Reverse proxy configured"
    echo
    
    # STEP 4: Test everything
    echo "🧪 Step 4: Testing connection..."
    sleep 2
    
    if curl -s --max-time 5 http://$SERVER_IP:8080 | grep -q "Success"; then
        echo
        echo "=========================================="
        echo "   ✅ SUCCESS! EVERYTHING WORKS!"
        echo "=========================================="
        echo
        echo "🌍 Access your PC from anywhere:"
        echo "   http://$SERVER_IP:8080"
        echo
        echo "Test it: curl http://$SERVER_IP:8080"
        echo
    else
        echo "   ⚠️  Connection test failed"
        echo
        echo "   Run diagnostics: testWireGuardProxy"
    fi
}

# Quick test function
testWireGuardProxy() {
    echo "Testing WireGuard connection..."
    echo
    
    if ping -c 2 -W 2 10.0.0.1 &>/dev/null; then
        echo "✓ PC can reach server (10.0.0.1)"
    else
        echo "✗ PC cannot reach server"
    fi
    
    if ssh root@31.133.102.195 "ping -c 2 -W 2 10.0.0.2" &>/dev/null; then
        echo "✓ Server can reach PC (10.0.0.2)"
    else
        echo "✗ Server cannot reach PC"
    fi
    
    echo
    echo "Testing web access..."
    
    if curl -s --max-time 5 http://31.133.102.195:8080 | head -n 1; then
        echo
        echo "✅ Web proxy is working!"
        echo "Access at: http://31.133.102.195:8080"
    else
        echo "✗ Web proxy failed"
        echo
        echo "Check: sudo wg show"
        echo "Check: sudo systemctl status nginx"
    fi
}
wireGuardRestart(){
    sudo wg-quick down wg0  # stop
    sudo wg-quick up wg0    # start
}

# Run on server to generate a client setup function for a new device
generateClientFunction() {
    read -p "Enter the name of the new device (e.g., rpi5): " CLIENT_NAME

    # Auto-assign next free IP based on existing peers
    BASE_IP="10.0.0."
    USED_IPS=$(grep -oP 'AllowedIPs = \K10\.0\.0\.\d+' /etc/wireguard/wg0.conf)
    NEXT_IP=2
    while echo "$USED_IPS" | grep -q "${BASE_IP}${NEXT_IP}"; do
        NEXT_IP=$((NEXT_IP+1))
    done
    CLIENT_IP="${BASE_IP}${NEXT_IP}"

    # Generate keys for client
    CLIENT_PRIV=$(wg genkey)
    CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)

    # Add peer to server config immediately
    echo -e "\n[Peer]\n# $CLIENT_NAME\nPublicKey = $CLIENT_PUB\nAllowedIPs = $CLIENT_IP/32" >> /etc/wireguard/wg0.conf
    wg-quick down wg0 2>/dev/null
    wg-quick up wg0

    SERVER_PUBLIC_KEY=$(wg show wg0 public-key)
    SERVER_PORT=$(grep '^ListenPort' /etc/wireguard/wg0.conf | awk '{print $3}')
    SERVER_IP=$(hostname -I | awk '{print $1}')

    # Echo the function to paste on the new device
    echo
    echo "Copy the following function to the new device and run it:"
    echo "---------------------------------------------------------"
    echo "setupWireGuardClient() {"
    echo "    PRIVATE_KEY=\"$CLIENT_PRIV\""
    echo "    CLIENT_IP=\"$CLIENT_IP/24\""
    echo "    SERVER_PUBLIC_KEY=\"$SERVER_PUBLIC_KEY\""
    echo "    SERVER_ENDPOINT=\"$SERVER_IP:$SERVER_PORT\""
    echo ""
    echo "    echo \"[Interface]\" > /etc/wireguard/wg0.conf"
    echo "    echo \"PrivateKey = \$PRIVATE_KEY\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"Address = \$CLIENT_IP\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"DNS = 10.0.0.1\" >> /etc/wireguard/wg0.conf"
    echo ""
    echo "    echo \"[Peer]\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"PublicKey = \$SERVER_PUBLIC_KEY\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"Endpoint = \$SERVER_ENDPOINT\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"AllowedIPs = 0.0.0.0/0, ::/0\" >> /etc/wireguard/wg0.conf"
    echo "    echo \"PersistentKeepalive = 25\" >> /etc/wireguard/wg0.conf"
    echo ""
    echo "    wg-quick down wg0 2>/dev/null || true"
    echo "    wg-quick up wg0"
    echo "    echo \"✅ WireGuard configured on this device ($CLIENT_NAME)\""
    echo "}"
    echo "---------------------------------------------------------"
}
