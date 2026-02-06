#!/bin/bash
# Audio Tunnel — streams local audio TO a remote peer's speakers
# Uses GStreamer with UDP/RTP/Opus for low-latency streaming.
# Follows the same peer-selector pattern as rdpPeer.sh.

AUDIO_PORT=4656
PIDFILE="/tmp/audioTunnel.pid"

# Check if a tunnel is already running
if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    ACTION=$(zenity --list \
        --title="Audio Tunnel" \
        --text="Audio tunnel is already running." \
        --column="Action" \
        "Stop tunnel" \
        "Keep running" \
        --width=300 --height=250)

    ACTION=$(echo "$ACTION" | cut -d'|' -f1)

    if [ "$ACTION" = "Stop tunnel" ]; then
        TUNNEL_PID=$(cat "$PIDFILE")
        REMOTE_PEER=$(cat /tmp/audioTunnel.peer 2>/dev/null)

        kill "$TUNNEL_PID" 2>/dev/null
        VOLSYNC_PID=$(cat /tmp/audioTunnel.volsync 2>/dev/null)
        [ -n "$VOLSYNC_PID" ] && kill "$VOLSYNC_PID" 2>/dev/null
        pkill -f "gst-launch.*audioTunnel" 2>/dev/null

        # Stop remote receiver
        if [ -n "$REMOTE_PEER" ]; then
            timeout 5 daemon send execOnPeer --peer "$REMOTE_PEER" --directory /tmp \
                --shellCmd "pkill -f 'gst-launch.*audioTunnel'" 2>/dev/null || true
        fi

        rm -f "$PIDFILE" /tmp/audioTunnel.peer /tmp/audioTunnel.volsync
        zenity --info --text="Audio tunnel stopped." --title="Audio Tunnel" --timeout=2
    fi
    exit 0
fi

# Query daemon for peers
PEERS_JSON=$(daemon send listPeers 2>/dev/null)

if [ -z "$PEERS_JSON" ] || [ "$PEERS_JSON" = "[]" ]; then
    zenity --error --text="No peers found.\nIs the daemon running?" --title="Audio Tunnel"
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
    zenity --error --text="No remote peers available." --title="Audio Tunnel"
    exit 1
fi

SELECTED=$(zenity --list \
    --title="Audio Tunnel" \
    --text="Stream audio TO which peer?" \
    --column="Peer" --column="IP" --column="Status" \
    --width=400 --height=350 \
    "${ZENITY_ARGS[@]}")

if [ -z "$SELECTED" ]; then
    exit 0
fi

# zenity may return "col1|col2|col3" — extract first field (peer_id)
SELECTED=$(echo "$SELECTED" | cut -d'|' -f1)

# Extract IP for selected peer
IP=$(echo "$PEERS_JSON" | jq -r --arg peer "$SELECTED" '.[] | select(.peer_id == $peer) | .ip_address')

# Find default sink's monitor device for capture
SINK_NAME=$(wpctl inspect @DEFAULT_AUDIO_SINK@ 2>/dev/null | grep 'node.name' | head -1 | grep -oP '"[^"]+"' | tr -d '"')
if [ -z "$SINK_NAME" ]; then
    zenity --error --text="Could not find default audio sink." --title="Audio Tunnel"
    exit 1
fi
MONITOR_DEV="${SINK_NAME}.monitor"

# Start remote receiver: GStreamer UDP/RTP/Opus → PulseAudio sink
timeout 10 daemon send execOnPeer --peer "$SELECTED" --directory /tmp \
    --shellCmd "nohup runuser -u \$(getent passwd 1000 | cut -d: -f1) -- env XDG_RUNTIME_DIR=/run/user/1000 gst-launch-1.0 -e udpsrc port=$AUDIO_PORT caps='application/x-rtp,media=audio,encoding-name=OPUS,payload=96' ! rtpjitterbuffer latency=150 ! rtpopusdepay ! opusdec plc=false ! audioconvert ! pulsesink buffer-time=40000 latency-time=10000 stream-properties='props,media.name=audioTunnel' >/dev/null 2>&1 &" 2>/dev/null

# Give remote receiver time to bind
sleep 1

# Start local sender: PulseAudio monitor → Opus → RTP → UDP
gst-launch-1.0 -e \
    pulsesrc device="$MONITOR_DEV" buffer-time=20000 latency-time=5000 \
    ! audioconvert ! audioresample \
    ! opusenc bitrate=128000 frame-size=5 \
    ! rtpopuspay ! queue ! udpsink host="$IP" port="$AUDIO_PORT" &
SENDER_PID=$!
echo "$SENDER_PID" > "$PIDFILE"
echo "$SELECTED" > /tmp/audioTunnel.peer

# Verify it started
sleep 1
if ! kill -0 "$SENDER_PID" 2>/dev/null; then
    rm -f "$PIDFILE" /tmp/audioTunnel.peer /tmp/audioTunnel.volsync
    daemon send execOnPeer --peer "$SELECTED" --directory /tmp \
        --shellCmd "pkill -f 'gst-launch.*audioTunnel'" 2>/dev/null
    zenity --error --text="Failed to start audio tunnel." --title="Audio Tunnel"
    exit 1
fi

# Volume + mute sync: mirror default sink to gst capture stream
(
    LAST_VOL=""
    LAST_MUTE=""
    sleep 2
    # Find the gst-launch stream node (under Streams section, not Clients)
    REC_NODE=$(wpctl status 2>/dev/null | sed -n '/Streams:/,/^$/p' | grep "gst-launch" | grep -oP '^\s+\K\d+' | head -1)
    [ -z "$REC_NODE" ] && exit 0
    while kill -0 "$SENDER_PID" 2>/dev/null; do
        VOL_LINE=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null)
        SINK_VOL=$(echo "$VOL_LINE" | awk '{print $2}')
        echo "$VOL_LINE" | grep -q MUTED && IS_MUTED=1 || IS_MUTED=0
        if [ -n "$SINK_VOL" ] && { [ "$SINK_VOL" != "$LAST_VOL" ] || [ "$IS_MUTED" != "$LAST_MUTE" ]; }; then
            wpctl set-volume "$REC_NODE" "$SINK_VOL" 2>/dev/null
            wpctl set-mute "$REC_NODE" "$IS_MUTED" 2>/dev/null
            LAST_VOL="$SINK_VOL"
            LAST_MUTE="$IS_MUTED"
        fi
        sleep 2
    done
) &
echo $! > /tmp/audioTunnel.volsync

zenity --info --text="Streaming audio to $SELECTED ($IP).\nRun again to stop." --title="Audio Tunnel" --timeout=3
