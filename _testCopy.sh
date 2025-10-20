# exec 2>/dev/null
# KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase2 domain=evsieveKeyboardPhase2 grab \
    --map key:a key:a key:s key:d key:f \
    --map key:q         @devNull \
    --map key:w         @devNull \
    --map key:rightctrl @devNull \
    --map key:leftctrl  @devNull \
    --map key:leftalt   @devNull \
    --map key:leftshift @devNull \
    --map key:backslash @devNull \
    --map key:enter     @devNull \
    --map key:e         @devNull \
    --map key:k         @devNull \
    --map key:y         @devNull \
    --map key:1         @devNull \
    --map key:2         @devNull \
    --map key:3         @devNull \
    --map key:4         @devNull \
    --map key:5         @devNull \
    --map key:6         @devNull \
    --hook key:e key:w send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
    --hook key:q key:e send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
    --hook key:q key:w send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
    --hook key:leftctrl key:leftalt key:leftshift key:1 send-key=key:enter send-key=key:g send-key=key:1 \
    --hook key:leftctrl key:leftalt key:leftshift key:2 send-key=key:enter send-key=key:g send-key=key:1 \
    --hook key:leftctrl key:leftalt key:leftshift key:3 send-key=key:enter send-key=key:g send-key=key:1 \
    --hook key:leftctrl key:leftalt key:leftshift key:4 send-key=key:enter \
    --hook key:leftctrl key:leftalt key:leftshift key:5 send-key=key:enter send-key=key:g send-key=key:1 \
    --hook key:leftctrl key:leftalt key:leftshift key:6 send-key=key:enter send-key=key:g send-key=key:1 \
    --output @devNull
    # (hook) sequential
    # --hook key:leftctrl key:e send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
    # (hook) --withhold
    # --hook key:q key:w exec-shell="echo Hello, world2!" \
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
qweqweqwasdfewasdf

