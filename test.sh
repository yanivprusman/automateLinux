
exec 2>/dev/null
# KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase2 domain=evsieveKeyboardPhase2 grab \
    --map key:a key:a key:s key:d key:f \
    --map key:q key:q@devNull \
    --map key:w key:w@devNull \
    --hook key:q key:w exec-shell="echo Hello, world2!" \
    --hook key:q key:w send-key=key:a send-key=key:s send-key=key:d send-key=key:f --withold \
    --output @devNull
    # --hook key:q key:w exec-shell="echo Hello, world2!" \
    # --withhold \
    # --print \
    # --hook key:enter exec-shell="echo Hello, world!" \
    # --map key:a key:b \
    # --print \
    # --hook key:q key:w exec-shell="echo Hello, world2!" \
    # --hook key:q key:w send-key=key:1 \
    # --map key:b key:a \
    # --map key:a key:a key:s key:d key:f \
    # --hook key:leftctrl exec-shell="echo Hello, world!" \
    # --hook key:leftctrl exec-shell="echo Hello, world!" \
    # --hook key:enter send-key=key:leftctrl:1 send-key=key:leftctrl:0 send-key=key:a \
    # --hook key:leftctrl send-key=key:a \
    # --hook key:leftctrl key:1 key:2 send-key=key:a \
qwqwasdf