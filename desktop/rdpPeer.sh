#!/bin/bash
# RDP Peer Selector — queries daemon for peers and lets user pick one

PEERS_JSON=$(daemon send listPeers 2>/dev/null)

if [ -z "$PEERS_JSON" ] || [ "$PEERS_JSON" = "[]" ]; then
    zenity --error --text="No peers found.\nIs the daemon running?" --title="RDP Peer Selector"
    exit 1
fi

MY_HOSTNAME=$(hostname)

# Build zenity list: peer_id, ip_address, status (skip self)
ZENITY_ARGS=()
while IFS= read -r line; do
    peer_id=$(echo "$line" | jq -r '.peer_id')
    ip=$(echo "$line" | jq -r '.ip_address')
    online=$(echo "$line" | jq -r '.is_online')

    # Skip self
    if [ "$peer_id" = "$MY_HOSTNAME" ]; then
        continue
    fi

    if [ "$online" = "true" ]; then
        status="Online"
    else
        status="Offline"
    fi

    ZENITY_ARGS+=("$peer_id" "$ip" "$status")
done < <(echo "$PEERS_JSON" | jq -c '.[]')

if [ ${#ZENITY_ARGS[@]} -eq 0 ]; then
    zenity --error --text="No remote peers available." --title="RDP Peer Selector"
    exit 1
fi

SELECTED=$(zenity --list \
    --title="RDP Peer Selector" \
    --text="Select a peer to connect to:" \
    --column="Peer" --column="IP" --column="Status" \
    --width=400 --height=350 \
    "${ZENITY_ARGS[@]}")

if [ -z "$SELECTED" ]; then
    exit 0
fi

# Extract IP for selected peer
IP=$(echo "$PEERS_JSON" | jq -r --arg peer "$SELECTED" '.[] | select(.peer_id == $peer) | .ip_address')

xfreerdp3 /v:"$IP":3389 /u:yaniv /p:automateLinux /f /smart-sizing /cert:tofu
