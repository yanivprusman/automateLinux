KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
EVSIEVE_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/log.txt")
SYSTEMD_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/error.txt")
> "$SYSTEMD_LOG_FILE"
for arg in "$@"; do
    if [[ "$arg" == "reset" ]]; then
        > "$EVSIEVE_LOG_FILE"
    fi
done
# systemd-run --service-type=notify --unit=corsairKeyBoard.service \
systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service \
    --property=StandardError=file:$SYSTEMD_LOG_FILE \
    --property=StandardOutput=append:$EVSIEVE_LOG_FILE \
    evsieve \
    --input /dev/input/by-id/$KEYBOARD_BY_ID /dev/input/$MOUSE_EVENT grab domain=input \
    `# toggle outputs` \
    --copy @input @regularOutput \
    --map @input @myOutput \
    --hook key:numlock toggle=myOutputNull toggle=regularOutputNull \
    --toggle @myOutput @myOutput @null id=myOutputNull \
    --toggle @regularOutput @null @regularOutput id=regularOutputNull \
    `# mouse events` \
    --map btn:forward key:enter \
    `# keyboard events` \
    --print key format=direct \
    --output @myOutput @regularOutput name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable
# --property=StandardOutput=file:$EVSIEVE_LOG_FILE \
#  > $EVSIEVE_LOG_FILE 2>$SYSTEMD_LOG_FILE
# | grep -v 'Quit' 
# --hook "" exec-shell='notify-send "Current time" "$(date '+%H:%M:%S')"' \
# f2        save
# f3        redo
# f4        undo
# compopse  comment
# G6         copy   
# G5         paste               
# G4         enter               
# G3         cut                 
# G2         select all and copy 
# G1         copy and search    
    # --map key:f2 key:f2@F \
    # --hook key:leftctrl key:f2 toggle=F \
    # --toggle @F @myF @regualrFWithoutCtrl id=F \
    # --print format=direct \
    # --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:f2:0 \
# hook to copy -> paste paste