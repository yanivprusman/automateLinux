KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')

sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
    --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 \
    --map key:f3:1 key:leftctrl:1 key:y:1 key:y:0 key:leftctrl:0 \
    --map key:f4:1 key:leftctrl:1 key:z:1 key:z:0 key:leftctrl:0 \
    --output name="evsieve keyboard"

    

    