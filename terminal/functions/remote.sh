_copyFileFromRemote(){
    mkdir -p /tmp/_copyFileFromRemote
    scp root@$kamateraIp:$1 /tmp/_copyFileFromRemote/
    local filename=$(basename "$1")
    if [ "$filename" = "wg0.conf" ]; then
        sed -i 's/^\(PrivateKey[[:space:]]*=[[:space:]]*\).*/\1<hidden>/' /tmp/_copyFileFromRemote/"$filename"
    fi    
}

printRemoteFiles(){
    local retrieve_flag=0
    local copy_flag=0
    for arg in "$@"; do
        case "$arg" in
            retrieve)
                retrieve_flag=1
                ;;
            -c)
                copy_flag=1
                ;;
            *)
                ;;
        esac
    done
    if [ $retrieve_flag -eq 1 ]; then
        getServerSettingsFromRemote
    fi
    local output=""
    local print_args=""
    if [ $copy_flag -eq 1 ]; then
        print_args="--no-color"
    fi
    output="$(print /tmp/_copyFileFromRemote/ $print_args)"
    output="files from the server:\n$output"
    if [ $copy_flag -eq 1 ]; then
        echo -e "$output" | copyToClipboard
    else
        echo -e "$output"
    fi
}

getServerSettingsFromRemote(){
    _copyFileFromRemote /etc/nginx/sites-available/pc-proxy
    _copyFileFromRemote /etc/wireguard/wg0.conf
}

sshKamatera(){
    ssh root@$kamateraIp
}

sshPhone(){
    ssh u0_a424@10.0.0.3 -p 8022
}

sshLaptop(){
    ssh root@$laptopIp
}

sshDesktop(){
    # ssh root@$pcIp
    ssh yaniv@$pcIp
}

# RDP to peer using gnome-remote-desktop
# Usage: rdp <peer> [password]
# Peers: desktop (10.0.0.2), laptop (10.0.0.4), rpi (10.0.0.6), vps (10.0.0.1), or IP address
rdp(){
    local peer="${1:-desktop}"
    local pass="${2:-automateLinux}"
    local ip

    case "$peer" in
        desktop|pc)
            ip="10.0.0.2"
            ;;
        laptop)
            ip="10.0.0.4"
            ;;
        rpi|rpi-ubuntu)
            ip="10.0.0.6"
            ;;
        vps)
            ip="10.0.0.1"
            ;;
        local|localhost)
            ip="127.0.0.1"
            ;;
        *)
            # Assume it's an IP address
            ip="$peer"
            ;;
    esac

    echo "Connecting to $peer ($ip)..."
    xfreerdp3 /v:"$ip":3389 /u:yaniv /p:"$pass" /f /smart-sizing &
}

