audioTunnel() {
    local AUDIO_PORT=4656
    local PIDFILE="/tmp/audioTunnel.pid"

    # Stop tunnel
    if [[ "$1" == "stop" ]]; then
        if [[ ! -f "$PIDFILE" ]] || ! kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
            echo "No audio tunnel running."
            return 0
        fi

        local tunnel_pid remote_peer
        tunnel_pid=$(cat "$PIDFILE")
        remote_peer=$(cat /tmp/audioTunnel.peer 2>/dev/null)

        kill "$tunnel_pid" 2>/dev/null
        # Kill volume sync loop
        local volsync_pid
        volsync_pid=$(cat /tmp/audioTunnel.volsync 2>/dev/null)
        [[ -n "$volsync_pid" ]] && kill "$volsync_pid" 2>/dev/null
        pkill -f "gst-launch.*audioTunnel" 2>/dev/null

        if [[ -n "$remote_peer" ]]; then
            timeout 5 d execOnPeer --peer "$remote_peer" --directory /tmp \
                --shellCmd "pkill -f 'gst-launch.*audioTunnel'" 2>/dev/null || echo "(remote cleanup timed out)"
        fi

        rm -f "$PIDFILE" /tmp/audioTunnel.peer /tmp/audioTunnel.volsync
        echo "Audio tunnel stopped."
        return 0
    fi

    # Check if already running
    if [[ -f "$PIDFILE" ]] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
        local running_peer
        running_peer=$(cat /tmp/audioTunnel.peer 2>/dev/null)
        echo "Audio tunnel already running (to $running_peer). Use 'audioTunnel stop' to stop."
        return 1
    fi

    local target_peer="$1"
    local peers_json
    peers_json=$(d listPeers 2>/dev/null)

    if [[ -z "$peers_json" || "$peers_json" == "[]" ]]; then
        echo "No peers found. Is the daemon running?" >&2
        return 1
    fi

    local my_hostname
    my_hostname=$(hostname)

    # If no peer specified, list and prompt
    if [[ -z "$target_peer" ]]; then
        local ids=() ips=()
        while IFS= read -r line; do
            local pid pip
            pid=$(echo "$line" | jq -r '.peer_id')
            pip=$(echo "$line" | jq -r '.ip_address')
            [[ "$pid" == "$my_hostname" ]] && continue
            ids+=("$pid")
            ips+=("$pip")
        done < <(echo "$peers_json" | jq -c '.[]')

        if [[ ${#ids[@]} -eq 0 ]]; then
            echo "No remote peers available." >&2
            return 1
        fi

        echo "Stream audio TO which peer?"
        local i
        for i in "${!ids[@]}"; do
            printf "  %d) %s (%s)\n" $((i + 1)) "${ids[$i]}" "${ips[$i]}"
        done
        printf "Select [1-%d]: " "${#ids[@]}"
        local choice
        read -r choice
        if [[ -z "$choice" || "$choice" -lt 1 || "$choice" -gt ${#ids[@]} ]] 2>/dev/null; then
            echo "Cancelled."
            return 0
        fi
        target_peer="${ids[$((choice - 1))]}"
    fi

    # Verify peer exists
    local ip
    ip=$(echo "$peers_json" | jq -r --arg p "$target_peer" '.[] | select(.peer_id == $p) | .ip_address')
    if [[ -z "$ip" ]]; then
        echo "Peer '$target_peer' not found in registry." >&2
        return 1
    fi

    # Find default sink's monitor device for capture
    local sink_name monitor_dev
    sink_name=$(wpctl inspect @DEFAULT_AUDIO_SINK@ 2>/dev/null | grep 'node.name' | head -1 | grep -oP '"[^"]+"' | tr -d '"')
    if [[ -z "$sink_name" ]]; then
        echo "Could not find default audio sink." >&2
        return 1
    fi
    monitor_dev="${sink_name}.monitor"

    # Start remote receiver: GStreamer UDP/RTP/Opus → PulseAudio sink
    echo "Starting receiver on $target_peer..."
    timeout 10 d execOnPeer --peer "$target_peer" --directory /tmp \
        --shellCmd "nohup runuser -u \$(getent passwd 1000 | cut -d: -f1) -- env XDG_RUNTIME_DIR=/run/user/1000 gst-launch-1.0 -e udpsrc port=$AUDIO_PORT caps='application/x-rtp,media=audio,encoding-name=OPUS,payload=96' ! rtpjitterbuffer latency=150 ! rtpopusdepay ! opusdec plc=false ! audioconvert ! pulsesink buffer-time=40000 latency-time=10000 stream-properties='props,media.name=audioTunnel' >/dev/null 2>&1 &" 2>/dev/null || echo "(receiver start timed out — may need manual start on peer)"

    sleep 1

    # Start local sender: PulseAudio monitor → Opus → RTP → UDP
    echo "Starting sender (capturing $monitor_dev)..."
    gst-launch-1.0 -e \
        pulsesrc device="$monitor_dev" buffer-time=20000 latency-time=5000 \
        ! audioconvert ! audioresample \
        ! opusenc bitrate=128000 frame-size=5 \
        ! rtpopuspay ! queue ! udpsink host="$ip" port="$AUDIO_PORT" &
    local sender_pid=$!
    echo "$sender_pid" > "$PIDFILE"
    echo "$target_peer" > /tmp/audioTunnel.peer

    sleep 1
    if ! kill -0 "$sender_pid" 2>/dev/null; then
        rm -f "$PIDFILE" /tmp/audioTunnel.peer /tmp/audioTunnel.volsync
        timeout 5 d execOnPeer --peer "$target_peer" --directory /tmp \
            --shellCmd "pkill -f 'gst-launch.*audioTunnel'" 2>/dev/null || true
        echo "Failed to start audio tunnel." >&2
        return 1
    fi

    # Volume + mute sync: mirror default sink to gst capture stream
    (
        local last_vol="" last_mute="" rec_node=""
        sleep 2
        # Find the gst-launch stream node (under Streams section, not Clients)
        rec_node=$(wpctl status 2>/dev/null | sed -n '/Streams:/,/^$/p' | grep "gst-launch" | grep -oP '^\s+\K\d+' | head -1)
        [[ -z "$rec_node" ]] && exit 0
        while kill -0 "$sender_pid" 2>/dev/null; do
            local vol_line sink_vol is_muted
            vol_line=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null)
            sink_vol=$(echo "$vol_line" | awk '{print $2}')
            [[ "$vol_line" == *MUTED* ]] && is_muted=1 || is_muted=0
            if [[ -n "$sink_vol" && ("$sink_vol" != "$last_vol" || "$is_muted" != "$last_mute") ]]; then
                wpctl set-volume "$rec_node" "$sink_vol" 2>/dev/null
                wpctl set-mute "$rec_node" "$is_muted" 2>/dev/null
                last_vol="$sink_vol"
                last_mute="$is_muted"
            fi
            sleep 2
        done
    ) &
    echo $! > /tmp/audioTunnel.volsync

    echo "Streaming audio to $target_peer ($ip). Use 'audioTunnel stop' to stop."
}
export -f audioTunnel
