# exec 2>/dev/null
# KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase2 domain=evsieveKeyboardPhase2 grab \
    --map key:a         @sparce \
    --map key:q         @sparce \
    --map key:w         @sparce \
    --map key:rightctrl @sparce \
    --map key:leftctrl  @sparce \
    --map key:leftalt   @sparce \
    --map key:leftshift @sparce \
    --map key:backslash @sparce \
    --map key:enter     @sparce \
    --map key:e         @sparce \
    --map key:k         @sparce \
    --map key:y         @sparce \
    --map key:1         @sparce \
    --map key:2         @sparce \
    --map key:3         @sparce \
    --map key:4         @sparce \
    --map key:5         @sparce \
    --map key:6         @sparce \
    --map key:tab         @sparce \
    --map key:e @devNull \
    --hook key:e key:w send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential\
    --withhold key:w \
\
    --hook key:leftshift key:e key:g key:k key:y key:4 send-key=key:enter sequential\
    --withhold \
    --output @sparce
# egky1
# egky2
# egky3
# egky4
# egky5
# egky6
# EGKY!
    # (hook) sequential
    # send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
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
qweqweqwasdfewasdf111

