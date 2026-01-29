#!/bin/bash
# setup_wireguard_peer.sh - Set up WireGuard client and register with daemon
# Can be run locally or piped via SSH from the daemon

set -e

# Constants
WG_SERVER_IP="31.133.102.195"
WG_SERVER_USER="root"
SSH_OPTS="-o StrictHostKeyChecking=accept-new -o ConnectTimeout=10"

# Parse arguments
NAME=""
VPN_IP=""
MAC=""
DUAL_BOOT=false
PRIVATE_KEY=""
LEADER_IP="10.0.0.2"

while [[ $# -gt 0 ]]; do
    case $1 in
        --name)
            NAME="$2"
            shift 2
            ;;
        --ip)
            VPN_IP="$2"
            shift 2
            ;;
        --mac)
            MAC="$2"
            shift 2
            ;;
        --dual-boot)
            DUAL_BOOT=true
            shift
            ;;
        --private-key)
            PRIVATE_KEY="$2"
            shift 2
            ;;
        --leader-ip)
            LEADER_IP="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

if [ -z "$NAME" ]; then
    echo "ERROR: --name is required"
    exit 1
fi

echo "=========================================="
echo "  WIREGUARD PEER SETUP: $NAME"
echo "=========================================="
echo

# Check SSH connectivity to WireGuard server
echo "Checking SSH connectivity to WireGuard server..."
if ! ssh $SSH_OPTS "$WG_SERVER_USER@$WG_SERVER_IP" "echo ok" &>/dev/null; then
    echo "ERROR: Cannot SSH to $WG_SERVER_USER@$WG_SERVER_IP"
    echo "Make sure you have SSH access to the WireGuard server first."
    exit 1
fi
echo "   SSH connection OK"
echo

# Handle key generation
if [ "$DUAL_BOOT" = true ]; then
    # Dual-boot: use provided private key
    if [ -z "$PRIVATE_KEY" ]; then
        echo "ERROR: --private-key is required for dual-boot setup"
        exit 1
    fi
    CLIENT_PRIV="$PRIVATE_KEY"
    CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
    echo "Using provided private key"
    echo "   Public key: $CLIENT_PUB"
else
    # New device: check for existing keys or generate new ones
    if [ -f /etc/wireguard/wg0.conf ]; then
        EXISTING_KEY=$(sudo grep -oP '^PrivateKey = \K.*' /etc/wireguard/wg0.conf 2>/dev/null || true)
        if [ -n "$EXISTING_KEY" ]; then
            CLIENT_PRIV="$EXISTING_KEY"
            CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
            echo "Reusing existing WireGuard key"
            echo "   Public key: $CLIENT_PUB"
        fi
    fi

    if [ -z "$CLIENT_PRIV" ]; then
        echo "Generating new WireGuard keys..."
        CLIENT_PRIV=$(wg genkey)
        CLIENT_PUB=$(echo "$CLIENT_PRIV" | wg pubkey)
        echo "   Public key: $CLIENT_PUB"
    fi
fi
echo

# Determine VPN IP
if [ -z "$VPN_IP" ]; then
    echo "Querying server for next available IP..."
    VPN_IP=$(ssh $SSH_OPTS "$WG_SERVER_USER@$WG_SERVER_IP" bash <<'REMOTE_SCRIPT'
BASE_IP="10.0.0."
USED_IPS=$(grep -oP 'AllowedIPs = \K10\.0\.0\.\d+' /etc/wireguard/wg0.conf 2>/dev/null)
NEXT_IP=2
while echo "$USED_IPS" | grep -q "${BASE_IP}${NEXT_IP}"; do
    NEXT_IP=$((NEXT_IP+1))
done
echo "${BASE_IP}${NEXT_IP}"
REMOTE_SCRIPT
)
    echo "   Assigned IP: $VPN_IP"
else
    echo "Using specified VPN IP: $VPN_IP"
fi
echo

# Fetch server info
echo "Fetching server configuration..."
SERVER_INFO=$(ssh $SSH_OPTS "$WG_SERVER_USER@$WG_SERVER_IP" bash <<'REMOTE_SCRIPT'
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
echo

# Register peer on server (skip for dual-boot since already registered)
if [ "$DUAL_BOOT" = false ]; then
    echo "Registering peer on WireGuard server..."
    ssh $SSH_OPTS "$WG_SERVER_USER@$WG_SERVER_IP" bash <<REMOTE_SCRIPT
# Check if peer already exists
if grep -q "# $NAME" /etc/wireguard/wg0.conf 2>/dev/null; then
    echo "   Peer already registered, updating..."
    # Remove existing peer block
    sudo sed -i "/# $NAME/,/^$/d" /etc/wireguard/wg0.conf
fi

# Add peer to server config
echo -e "\n[Peer]\n# $NAME\nPublicKey = $CLIENT_PUB\nAllowedIPs = $VPN_IP/32" >> /etc/wireguard/wg0.conf

# Reload WireGuard
wg-quick down wg0 2>/dev/null || true
wg-quick up wg0
REMOTE_SCRIPT
    echo "   Peer registered on server"
fi
echo

# Create local config
echo "Creating local WireGuard config..."
sudo bash -c "cat > /etc/wireguard/wg0.conf" <<EOF
[Interface]
PrivateKey = $CLIENT_PRIV
Address = $VPN_IP/24

[Peer]
PublicKey = $SERVER_PUBLIC_KEY
Endpoint = $WG_SERVER_IP:$SERVER_PORT
AllowedIPs = 10.0.0.0/24
PersistentKeepalive = 25
EOF
echo "   Config written to /etc/wireguard/wg0.conf"
echo

# Start WireGuard
echo "Starting WireGuard..."
sudo wg-quick down wg0 2>/dev/null || true
sudo wg-quick up wg0
sudo systemctl enable wg-quick@wg0 2>/dev/null || true
echo

# Test connection
echo "Testing connection to VPN network..."
sleep 2
if ping -c 2 -W 3 10.0.0.1 &>/dev/null; then
    echo "   VPN connection successful!"
else
    echo "   WARNING: VPN ping test failed"
    echo "   Check: sudo wg show"
fi
echo

# Set up SSH key if needed
echo "Setting up SSH key authentication..."
SSH_KEY="$HOME/.ssh/id_ed25519"
if [ ! -f "$SSH_KEY" ]; then
    echo "   Generating SSH key..."
    mkdir -p "$HOME/.ssh"
    chmod 700 "$HOME/.ssh"
    ssh-keygen -t ed25519 -f "$SSH_KEY" -N "" -q
fi

if ssh-copy-id $SSH_OPTS "$WG_SERVER_USER@$WG_SERVER_IP" &>/dev/null; then
    echo "   SSH key copied to WireGuard server"
else
    echo "   SSH key may already exist on server"
fi
echo

# Register with daemon peer system
echo "Registering with daemon peer system..."
if command -v daemon &>/dev/null; then
    DAEMON_CMD="daemon"
elif [ -S "/run/automatelinux/automatelinux-daemon.sock" ]; then
    DAEMON_CMD="echo '{\"command\":\"setPeerConfig\",\"role\":\"worker\",\"id\":\"$NAME\",\"leader\":\"$LEADER_IP\"}' | nc -U /run/automatelinux/automatelinux-daemon.sock"
else
    echo "   WARNING: Daemon not found, skipping peer registration"
    echo "   Run manually: d setPeerConfig --role worker --id $NAME --leader $LEADER_IP"
    DAEMON_CMD=""
fi

if [ -n "$DAEMON_CMD" ]; then
    if [ "$DAEMON_CMD" = "daemon" ]; then
        daemon send setPeerConfig --role worker --id "$NAME" --leader "$LEADER_IP" 2>/dev/null || \
        echo '{"command":"setPeerConfig","role":"worker","id":"'"$NAME"'","leader":"'"$LEADER_IP"'"}' | nc -U /run/automatelinux/automatelinux-daemon.sock 2>/dev/null || \
        echo "   WARNING: Could not register with daemon"
    else
        eval "$DAEMON_CMD" 2>/dev/null || echo "   WARNING: Could not register with daemon"
    fi
fi
echo

echo "=========================================="
echo "   SETUP COMPLETE"
echo "=========================================="
echo
echo "   Peer name:  $NAME"
echo "   VPN IP:     $VPN_IP"
echo "   Leader:     $LEADER_IP"
echo
echo "   To restart WireGuard: sudo wg-quick down wg0 && sudo wg-quick up wg0"
echo "   To check status: sudo wg show"
echo
