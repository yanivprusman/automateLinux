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
    ssh root@$pcIp
}

# RDP to peer using gnome-remote-desktop
# Usage: rdp <peer> [password]
# Peers: desktop (10.0.0.2), laptop (10.0.0.4), vps (10.0.0.1), or IP address
rdp(){
    local peer="${1:-desktop}"
    local pass="${2:-testpass123}"
    local ip

    case "$peer" in
        desktop|pc)
            ip="10.0.0.2"
            ;;
        laptop)
            ip="10.0.0.4"
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

# Set up RDP credentials on the local machine (run on the target peer)
# Usage: setupRdp [user] [password]
setupRdp(){
    local user="${1:-$(whoami)}"
    local pass="${2:-testpass123}"

    # Wipe keyring files (both old .keyring and new .keystore formats)
    rm -f ~/.local/share/keyrings/*.keyring ~/.local/share/keyrings/*.keystore

    # Restart keyring daemon with fresh state
    pkill -u "$(id -u)" gnome-keyring-daemon 2>/dev/null || true
    sleep 0.5
    eval "$(printf '\0' | gnome-keyring-daemon --start --login --components=secrets 2>/dev/null)"
    sleep 1

    # Create the login collection via gnome-keyring internal D-Bus API (no prompt)
    python3 -c "
import dbus
bus = dbus.SessionBus()
svc = bus.get_object('org.freedesktop.secrets', '/org/freedesktop/secrets')
si = dbus.Interface(svc, 'org.freedesktop.Secret.Service')
_, session = si.OpenSession('plain', dbus.String('', variant_level=1))
ki = dbus.Interface(svc, 'org.gnome.keyring.InternalUnsupportedGuiltRiddenInterface')
props = dbus.Dictionary({'org.freedesktop.Secret.Collection.Label': 'Login'}, signature='sv')
secret = dbus.Struct((session, dbus.ByteArray(b''), dbus.ByteArray(b''), 'text/plain'), signature='oayays')
col = ki.CreateWithMasterPassword(props, secret)
si.SetAlias('default', col)
print('Login collection created')
" 2>&1

    grdctl rdp set-credentials "$user" "$pass"
    systemctl --user restart gnome-remote-desktop

    sleep 1
    if ss -tlnp 2>/dev/null | grep -q ":3389 "; then
        echo "RDP ready on port 3389 (user: $user)"
    else
        echo "RDP not listening — try logging out and back in"
    fi
}
