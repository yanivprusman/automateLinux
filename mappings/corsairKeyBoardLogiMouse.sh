KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
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
# systemd-run --service-type=notify --unit=corsairKeyBoard.service \
systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service \
    evsieve \
    --input /dev/input/by-id/$KEYBOARD_BY_ID grab domain=regular \
    --input /dev/input/$MOUSE_EVENT grab \
    --map btn:forward key:enter \
    --output name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable 
    # --hook "" exec-shell='notify-send "Current time" "$(date '+%H:%M:%S')"' \


