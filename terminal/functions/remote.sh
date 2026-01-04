_copyFileFromRemote(){
    mkdir -p /tmp/_copyFileFromRemote
    scp root@10.0.0.1:$1 /tmp/_copyFileFromRemote/
    local filename=$(basename "$1")
    if [ "$filename" = "wg0.conf" ]; then
        sed -i 's/^\(PrivateKey[[:space:]]*=[[:space:]]*\).*/\1<hidden>/' /tmp/_copyFileFromRemote/"$filename"
    fi    
}

# printRemoteFiles(){
#     echo files from the server 
#     print /tmp/_copyFileFromRemote/ $@
# }

# printRemoteFiles(){
#     local retrieve_flag=0
#     for arg in "$@"; do
#         if [ "$arg" = "retrieve" ]; then
#             retrieve_flag=1
#         else
#             other_args+=("$arg")
#         fi
#     done
#     echo "files from the server"
#     if [ $retrieve_flag -eq 1 ]; then
#         getServerSettingsFromRemote
#         return
#     fi
#     print /tmp/_copyFileFromRemote/ "${other_args[@]}"
# }

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
                # Ignore other arguments as they are not passed on
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

    # Always prepend header
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
    ssh root@$phoneIp
}

sshLaptop(){
    ssh root@$laptopIp
}
