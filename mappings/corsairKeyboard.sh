KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
systemd-run --service-type=notify --unit=automateLinuxKeyboardPhase1.service \
    evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
    --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:f2:0 \
    --map key:f3:1 key:leftctrl:1 key:y:1 key:y:0 key:leftctrl:0 key:f3:0 \
    --map key:f4:1 key:leftctrl:1 key:z:1 key:z:0 key:leftctrl:0 key:f4:0 \
    --map key:f5:1 key:s:1 key:s:0 key:f5:0 key:leftctrl:0 \
    --map key:f6:1 key:s:1 key:s:0 key:f6:0 \
    --map key:compose:1 key:leftctrl:1 key:slash:1 key:slash:0 key:leftctrl:0 key:compose:0 \
    --output name="keyboard" create-link=/dev/input/by-id/keyBoard repeat=disable 
