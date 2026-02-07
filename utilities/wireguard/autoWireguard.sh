#!/bin/bash
# PC-side script to fully automate WireGuard setup

# Check root
if [[ $EUID -ne 0 ]]; then
    echo "Run as root"
    exit 1
fi

# Ask for info
read -p "Enter your server IP: " SERVER_IP
read -p "Enter your server SSH username: " SERVER_USER

# Install WireGuard on PC
apt update && apt install -y wireguard

# Run setup commands on server via SSH
ssh $SERVER_USER@$SERVER_IP "bash -s" <<EOF
# Install WireGuard
sudo apt update && sudo apt install -y wireguard

# Generate keys
SERVER_PRIV=\$(wg genkey)
SERVER_PUB=\$(printf "%s" "\$SERVER_PRIV" | wg pubkey)
CLIENT_PRIV=\$(wg genkey)
CLIENT_PUB=\$(printf "%s" "\$CLIENT_PRIV" | wg pubkey)

# Server config
sudo tee /etc/wireguard/wg0.conf >/dev/null <<EOC
[Interface]
PrivateKey = \$SERVER_PRIV
Address = 10.0.0.1/24
ListenPort = 51820

[Peer]
PublicKey = \$CLIENT_PUB
AllowedIPs = 10.0.0.2/32
EOC

# Enable IP forwarding
echo "net.ipv4.ip_forward=1" | sudo tee /etc/sysctl.d/99-wireguard.conf
sudo sysctl --system

# Start WireGuard
sudo wg-quick up wg0
sudo systemctl enable wg-quick@wg0

# Save client config
cat > ~/wg0-client.conf <<EOC
[Interface]
PrivateKey = \$CLIENT_PRIV
Address = 10.0.0.2/24

[Peer]
PublicKey = \$SERVER_PUB
Endpoint = $SERVER_IP:51820
AllowedIPs = 0.0.0.0/0
EOC
EOF

# Copy client config from server to PC
scp $SERVER_USER@$SERVER_IP:~/wg0-client.conf /etc/wireguard/wg0.conf

# Start WireGuard on PC
sudo wg-quick up wg0
sudo systemctl enable wg-quick@wg0

echo "WireGuard setup complete. Tunnel is active."
