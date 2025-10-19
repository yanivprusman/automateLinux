KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
    --output name="evsieve keyboard phase 1" create-link=/dev/input/by-id/evsieveKeyboardPhase1 &

sleep 1

sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase1 grab \
    --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:f2:0 \
    --map key:f3:1 key:leftctrl:1 key:y:1 key:y:0 key:leftctrl:0 key:f3:0 \
    --map key:f4:1 key:leftctrl:1 key:z:1 key:z:0 key:leftctrl:0 key:f4:0 \
    --map key:f5:1 key:s:1 key:s:0 key:f5:0 key:leftctrl:0 \
    --map key:f6:1 key:s:1 key:s:0 key:f6:0 \
    --map key:compose:1 key:leftctrl:1 key:slash:1 key:slash:0 key:leftctrl:0 key:compose:0 \
    --output name="evsieve keyboard phase 2" create-link=/dev/input/by-id/evsieveKeyboardPhase2 &

wait
    # --hook key:leftctrl key:n send-key=key:f5 sequential --withhold \
    # --toggle key:a key:b \
 
    