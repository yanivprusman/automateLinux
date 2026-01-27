---
name: wireguard
description: WireGuard VPN setup and management functions for peer configuration and proxy tunneling
---

# WireGuard Management

This skill provides functions for setting up and managing WireGuard VPN connections between your machines and a remote server.

## Configuration

Constants defined in `terminal/functions/wireguard.sh`:
- `WG_SERVER_IP`: Server IP address (31.133.102.195)
- `WG_SERVER_USER`: SSH user for server (root)

## Functions

### `setupWireGuardPeer`

Sets up WireGuard on a new peer machine. Handles both new devices and dual-boot scenarios.

**Usage:**
```bash
setupWireGuardPeer
```

**Interactive prompts:**
1. New device vs dual-boot selection
2. For new devices: device name, optional key reuse
3. For dual-boot: existing private key and IP

**What it does:**
- Generates or reuses WireGuard keys
- Queries server for next available IP (new devices)
- Registers peer on server (new devices only)
- Creates `/etc/wireguard/wg0.conf`
- Starts WireGuard and enables on boot

### `setupWireGuardProxyFromScratch`

Complete setup for exposing local services via the WireGuard server. Run from your PC.

**Usage:**
```bash
setupWireGuardProxyFromScratch
```

**What it does:**
1. Syncs WireGuard keys between PC and server
2. Creates test web page on PC (`~/wireguard-test-site`)
3. Configures nginx on PC (port 80)
4. Configures nginx reverse proxy on server (port 8080 -> 10.0.0.2:80)
5. Tests the connection

**Result:** Access your PC at `http://31.133.102.195:8080`

### `wireGuardRestart`

Restarts the WireGuard interface.

**Usage:**
```bash
wireGuardRestart
```

### `testWireGuardProxy`

Tests WireGuard connectivity and web proxy.

**Usage:**
```bash
testWireGuardProxy
```

**Checks:**
- PC can reach server (10.0.0.1)
- Server can reach PC (10.0.0.2)
- Web proxy is accessible

## Network Layout

```
Internet
    |
Server (31.133.102.195)
    |  WireGuard: 10.0.0.1
    |  nginx: 8080 -> 10.0.0.2:80
    |
[WireGuard Tunnel]
    |
PC (10.0.0.2)
    |  nginx: 80
```

## Troubleshooting

- **Check WireGuard status:** `sudo wg show`
- **Check nginx:** `sudo systemctl status nginx`
- **Emergency restart:** `wireGuardRestart`
- **View config:** `sudo cat /etc/wireguard/wg0.conf`
