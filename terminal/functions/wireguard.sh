# Constants
WG_SERVER_IP="31.133.102.195"
WG_SERVER_USER="root"

# Complete WireGuard Proxy Setup - Run this once from your PC
setupWireGuardProxyFromScratch() {
    local SERVER_USER="$WG_SERVER_USER"
    local SERVER_IP="$WG_SERVER_IP"
    local SSH_OPTS="-o StrictHostKeyChecking=accept-new -o ConnectTimeout=5"

    echo "=========================================="
    echo "  COMPLETE WIREGUARD PROXY SETUP"
    echo "=========================================="
    echo
    
    # STEP 1: Fix WireGuard keys to match reality
    echo "🔑 Step 1: Syncing WireGuard keys..."
    
    local SERVER_ACTUAL_KEY=$(ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" "wg show wg0 public-key 2>/dev/null")

    if [ -z "$SERVER_ACTUAL_KEY" ]; then
        echo "   Starting WireGuard on server first..."
        ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" "wg-quick up wg0 2>/dev/null"
        sleep 2
        SERVER_ACTUAL_KEY=$(ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" "wg show wg0 public-key")
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

    ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" bash << 'EOF'
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
    local SSH_OPTS="-o StrictHostKeyChecking=accept-new -o ConnectTimeout=5"

    echo "Testing WireGuard connection..."
    echo

    if ping -c 2 -W 2 10.0.0.1 &>/dev/null; then
        echo "✓ PC can reach server (10.0.0.1)"
    else
        echo "✗ PC cannot reach server"
    fi

    if ssh $SSH_OPTS $WG_SERVER_USER@$WG_SERVER_IP "ping -c 2 -W 2 10.0.0.2" &>/dev/null; then
        echo "✓ Server can reach PC (10.0.0.2)"
    else
        echo "✗ Server cannot reach PC"
    fi
    
    echo
    echo "Testing web access..."
    
    if curl -s --max-time 5 http://$WG_SERVER_IP:8080 | head -n 1; then
        echo
        echo "✅ Web proxy is working!"
        echo "Access at: http://$WG_SERVER_IP:8080"
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

# Run on the PEER machine to set up WireGuard client
# Handles both new devices and dual-boot scenarios
setupWireGuardPeer() {
    local SERVER_USER="$WG_SERVER_USER"
    local SERVER_IP="$WG_SERVER_IP"
    local IS_DUAL_BOOT=false
    local SSH_OPTS="-o StrictHostKeyChecking=accept-new -o ConnectTimeout=5"

    echo "=========================================="
    echo "  WIREGUARD PEER SETUP (run from peer)"
    echo "=========================================="
    echo

    # Check SSH connectivity first
    echo "Checking SSH connectivity to server..."
    if ! ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" "echo ok" &>/dev/null; then
        echo "ERROR: Cannot SSH to $SERVER_USER@$SERVER_IP"
        echo "Make sure you have SSH access to the server first."
        return 1
    fi
    echo "   SSH connection OK"
    echo

    # Ask if this is a new device or dual-boot
    echo "Is this a new device or dual-boot (reusing existing peer)?"
    echo "  1) New device (generate new keys, auto-assign IP)"
    echo "  2) Dual-boot / existing peer (reuse keys and IP)"
    read -p "Choice [1/2]: " SETUP_CHOICE

    if [ "$SETUP_CHOICE" = "2" ]; then
        IS_DUAL_BOOT=true
    fi

    if [ "$IS_DUAL_BOOT" = true ]; then
        # Dual-boot: get existing private key and IP
        echo
        echo "Enter the Private Key from your existing config (e.g., Windows side):"
        read -s CLIENT_PRIV
        echo
        if [ -z "$CLIENT_PRIV" ]; then
            echo "ERROR: Private key is required for dual-boot setup"
            return 1
        fi
        CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
        echo "   Public key: $CLIENT_PUB"

        echo
        read -p "Enter the IP address to use (e.g., 10.0.0.4): " CLIENT_IP
        if [ -z "$CLIENT_IP" ]; then
            echo "ERROR: IP address is required"
            return 1
        fi
    else
        # New device: prompt for name
        read -p "Enter a name for this device (e.g., rpi5, laptop): " CLIENT_NAME
        if [ -z "$CLIENT_NAME" ]; then
            echo "ERROR: Device name required"
            return 1
        fi

        # Check if config already exists and offer to reuse keys
        if [ -f /etc/wireguard/wg0.conf ]; then
            EXISTING_KEY=$(sudo grep -oP '^PrivateKey = \K.*' /etc/wireguard/wg0.conf 2>/dev/null)
            if [ -n "$EXISTING_KEY" ]; then
                EXISTING_PUB=$(echo "$EXISTING_KEY" | wg pubkey)
                echo
                echo "Found existing config with public key: $EXISTING_PUB"
                read -p "Reuse existing key? [Y/n]: " REUSE_KEY
                if [ "$REUSE_KEY" != "n" ] && [ "$REUSE_KEY" != "N" ]; then
                    CLIENT_PRIV="$EXISTING_KEY"
                    CLIENT_PUB="$EXISTING_PUB"
                    echo "   Reusing existing key"
                else
                    echo
                    echo "Generating NEW WireGuard keys..."
                    CLIENT_PRIV=$(wg genkey)
                    CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
                    echo "   Public key: $CLIENT_PUB"
                fi
            else
                echo
                echo "Generating WireGuard keys locally..."
                CLIENT_PRIV=$(wg genkey)
                CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
                echo "   Public key: $CLIENT_PUB"
            fi
        else
            echo
            echo "Generating WireGuard keys locally..."
            CLIENT_PRIV=$(wg genkey)
            CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
            echo "   Public key: $CLIENT_PUB"
        fi
        echo "   Public key: $CLIENT_PUB"

        echo
        echo "Querying server for next available IP..."
        CLIENT_IP=$(ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" bash <<'REMOTE_SCRIPT'
BASE_IP="10.0.0."
USED_IPS=$(grep -oP 'AllowedIPs = \K10\.0\.0\.\d+' /etc/wireguard/wg0.conf 2>/dev/null)
NEXT_IP=2
while echo "$USED_IPS" | grep -q "${BASE_IP}${NEXT_IP}"; do
    NEXT_IP=$((NEXT_IP+1))
done
echo "${BASE_IP}${NEXT_IP}"
REMOTE_SCRIPT
)
        echo "   Assigned IP: $CLIENT_IP"
    fi

    # Fetch server info (always needed)
    echo
    echo "Fetching server configuration..."
    SERVER_INFO=$(ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" bash <<'REMOTE_SCRIPT'
SERVER_PUBLIC_KEY=$(wg show wg0 public-key 2>/dev/null)
if [ -z "$SERVER_PUBLIC_KEY" ]; then
    wg-quick up wg0 2>/dev/null
    sleep 1
    SERVER_PUBLIC_KEY=$(wg show wg0 public-key)
fi
SERVER_PORT=$(grep '^ListenPort' /etc/wireguard/wg0.conf | awk '{print $3}')
SERVER_PORT=${SERVER_PORT:-51820}
echo "$SERVER_PUBLIC_KEY|$SERVER_PORT"
REMOTE_SCRIPT
)
    SERVER_PUBLIC_KEY=$(echo "$SERVER_INFO" | cut -d'|' -f1)
    SERVER_PORT=$(echo "$SERVER_INFO" | cut -d'|' -f2)
    echo "   Server public key: $SERVER_PUBLIC_KEY"
    echo "   Server port: $SERVER_PORT"

    # Register peer on server (only for new devices)
    if [ "$IS_DUAL_BOOT" = false ]; then
        echo
        echo "Registering peer on server..."
        ssh $SSH_OPTS "$SERVER_USER@$SERVER_IP" bash <<REMOTE_SCRIPT
# Add peer to server config
echo -e "\n[Peer]\n# $CLIENT_NAME\nPublicKey = $CLIENT_PUB\nAllowedIPs = $CLIENT_IP/32" >> /etc/wireguard/wg0.conf

# Reload WireGuard
wg-quick down wg0 2>/dev/null
wg-quick up wg0
REMOTE_SCRIPT
        echo "   Peer registered on server"
    fi

    # Create local config
    echo
    echo "Creating local WireGuard config..."
    sudo bash -c "cat > /etc/wireguard/wg0.conf" <<EOF
[Interface]
PrivateKey = $CLIENT_PRIV
Address = $CLIENT_IP/24

[Peer]
PublicKey = $SERVER_PUBLIC_KEY
Endpoint = $SERVER_IP:$SERVER_PORT
AllowedIPs = 10.0.0.0/24
PersistentKeepalive = 25
EOF
    echo "   Config written to /etc/wireguard/wg0.conf"

    # Start WireGuard
    echo
    echo "Starting WireGuard..."
    sudo wg-quick down wg0 2>/dev/null || true
    sudo wg-quick up wg0
    sudo systemctl enable wg-quick@wg0

    # Test connection
    echo
    echo "Testing connection to server..."
    sleep 2
    if ping -c 2 -W 3 10.0.0.1 &>/dev/null; then
        echo
        echo "=========================================="
        echo "   SUCCESS! WireGuard is connected"
        echo "=========================================="
        echo
        echo "   Your VPN IP: $CLIENT_IP"
        echo "   Server VPN IP: 10.0.0.1"
        echo
        echo "   To restart: wireGuardRestart"
        echo "   To test: ping 10.0.0.1"
    else
        echo
        echo "   Connection test failed."
        echo "   Check: sudo wg show"
        echo "   Check server: ssh $SERVER_USER@$SERVER_IP 'wg show'"
    fi
}
