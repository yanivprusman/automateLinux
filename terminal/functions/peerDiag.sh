peerDiag() {
    local target_peer="$1"

    # Colors
    local green="\e[32m"
    local red="\e[31m"
    local yellow="\e[33m"
    local bold="\e[1m"
    local dim="\e[2m"
    local reset="\e[0m"

    local pass="${green}PASS${reset}"
    local fail="${red}FAIL${reset}"
    local skip="${dim}SKIP${reset}"

    _peerDiag_step() {
        local status="$1"
        local label="$2"
        local detail="$3"
        printf "  %-6b %-35s %s\n" "$status" "$label" "$detail"
    }

    # Step 1: Check local daemon
    local peer_status
    peer_status=$(d getPeerStatus 2>/dev/null)
    if [[ -z "$peer_status" ]]; then
        echo -e "${red}Local daemon is not running or not responding.${reset}"
        echo "Start it with: sudo systemctl start daemon.service"
        return 1
    fi

    local local_role local_id local_leader
    local_role=$(echo "$peer_status" | jq -r '.role')
    local_id=$(echo "$peer_status" | jq -r '.peer_id')
    local_leader=$(echo "$peer_status" | jq -r '.connected_to_leader')

    # Get peer list
    local peers_json
    peers_json=$(d listPeers 2>/dev/null)
    if [[ -z "$peers_json" || "$peers_json" == "null" ]]; then
        echo -e "${red}Could not retrieve peer list.${reset}"
        return 1
    fi

    # Build list of peers to check
    local peer_ids=()
    if [[ -n "$target_peer" ]]; then
        # Verify target exists
        local exists
        exists=$(echo "$peers_json" | jq -r --arg p "$target_peer" '[.[] | select(.peer_id == $p)] | length')
        if [[ "$exists" == "0" ]]; then
            echo -e "${red}Peer '$target_peer' not found in registry.${reset}"
            echo "Registered peers:"
            echo "$peers_json" | jq -r '.[].peer_id' | sed 's/^/  /'
            return 1
        fi
        peer_ids+=("$target_peer")
    else
        while IFS= read -r pid; do
            peer_ids+=("$pid")
        done < <(echo "$peers_json" | jq -r '.[].peer_id')
    fi

    echo -e "${bold}Local daemon:${reset} ${local_role} (${local_id}), leader connected: ${local_leader}"
    echo ""

    for peer_id in "${peer_ids[@]}"; do
        local ip last_seen is_online
        ip=$(echo "$peers_json" | jq -r --arg p "$peer_id" '.[] | select(.peer_id == $p) | .ip_address')
        last_seen=$(echo "$peers_json" | jq -r --arg p "$peer_id" '.[] | select(.peer_id == $p) | .last_seen')
        is_online=$(echo "$peers_json" | jq -r --arg p "$peer_id" '.[] | select(.peer_id == $p) | .is_online')

        echo -e "${bold}[$peer_id]${reset} ${dim}($ip)${reset}"

        # Track results for summary
        local reg_ok=1 fresh_ok=0 ping_ok=0 tcp_ok=0 exec_ok=0 leader_ok=0
        local staleness_str=""

        # Step: Registry
        _peerDiag_step "$pass" "Registered" "is_online=$is_online"

        # Step: last_seen freshness
        if [[ "$last_seen" != "null" && -n "$last_seen" ]]; then
            local last_epoch now_epoch diff_secs
            last_epoch=$(date -d "$last_seen" +%s 2>/dev/null)
            now_epoch=$(date +%s)
            if [[ -n "$last_epoch" ]]; then
                diff_secs=$((now_epoch - last_epoch))
                if [[ $diff_secs -lt 60 ]]; then
                    staleness_str="${diff_secs}s ago"
                    fresh_ok=1
                elif [[ $diff_secs -lt 3600 ]]; then
                    staleness_str="$((diff_secs / 60))m ago"
                    fresh_ok=1
                elif [[ $diff_secs -lt 86400 ]]; then
                    staleness_str="$((diff_secs / 3600))h ago"
                    # Under 24h is still plausible (no heartbeat, only set on connect)
                    fresh_ok=1
                else
                    staleness_str="$((diff_secs / 86400))d ago"
                fi
                if [[ $fresh_ok -eq 1 ]]; then
                    _peerDiag_step "$pass" "last_seen fresh" "$staleness_str ($last_seen)"
                else
                    _peerDiag_step "$fail" "last_seen stale" "$staleness_str ($last_seen)"
                fi
            else
                _peerDiag_step "$fail" "last_seen" "could not parse: $last_seen"
            fi
        else
            _peerDiag_step "$fail" "last_seen" "no timestamp"
        fi

        # Step: WireGuard ping
        if ping -c 2 -W 2 "$ip" &>/dev/null; then
            ping_ok=1
            _peerDiag_step "$pass" "WireGuard ping" ""
        else
            _peerDiag_step "$fail" "WireGuard ping" "no response from $ip"
        fi

        # Step: TCP port 3502
        if [[ $ping_ok -eq 1 ]]; then
            if nc -z -w 3 "$ip" 3502 2>/dev/null; then
                tcp_ok=1
                _peerDiag_step "$pass" "TCP :3502" ""
            else
                _peerDiag_step "$fail" "TCP :3502" "port not open"
            fi
        else
            _peerDiag_step "$skip" "TCP :3502" "skipped (no ping)"
        fi

        # Step: Daemon exec roundtrip
        if [[ $tcp_ok -eq 1 ]]; then
            local exec_result
            exec_result=$(d execOnPeer --peer "$peer_id" --directory /tmp --shellCmd "echo peerDiag_ok" 2>&1)
            if echo "$exec_result" | grep -q "peerDiag_ok"; then
                exec_ok=1
                _peerDiag_step "$pass" "Daemon exec" ""
            else
                local short_err
                short_err=$(echo "$exec_result" | head -1 | cut -c1-60)
                _peerDiag_step "$fail" "Daemon exec" "$short_err"
            fi
        else
            _peerDiag_step "$skip" "Daemon exec" "skipped (no TCP)"
        fi

        # Step: Leader reachable (only for non-leader peers)
        if [[ "$ip" != "10.0.0.1" ]]; then
            if nc -z -w 3 10.0.0.1 3502 2>/dev/null; then
                leader_ok=1
                _peerDiag_step "$pass" "Leader :3502" ""
            else
                _peerDiag_step "$fail" "Leader :3502" "leader not reachable"
            fi
        else
            leader_ok=1
            _peerDiag_step "$skip" "Leader :3502" "this is the leader"
        fi

        # Summary diagnosis
        local diagnosis=""
        if [[ $exec_ok -eq 1 ]]; then
            diagnosis="${green}Peer fully reachable${reset}"
        elif [[ $tcp_ok -eq 1 ]]; then
            diagnosis="${yellow}TCP open but exec failed (daemon may be busy or erroring)${reset}"
        elif [[ $ping_ok -eq 1 && $tcp_ok -eq 0 ]]; then
            diagnosis="${red}VPN up but daemon not listening on peer${reset}"
        elif [[ $ping_ok -eq 0 ]]; then
            diagnosis="${red}WireGuard tunnel down${reset}"
        fi

        if [[ $leader_ok -eq 0 && "$ip" != "10.0.0.1" ]]; then
            diagnosis="${diagnosis:+$diagnosis, }${red}leader unreachable${reset}"
        fi

        echo -e "  -> $diagnosis"
        echo ""
    done
}
export -f peerDiag
