# exec 2>/dev/null
# KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase2 domain=evsieveKeyboardPhase2 grab \
    --map key:a         @sparce \
    --map key:q         @sparce \
    --map key:w         @sparce \
    --map key:rightctrl @sparce \
    --map key:leftctrl  @ctrl \
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
    --map key:tab       @sparce \
    --map key:e         @sparce \
    \
    --hook key:leftshift key:w send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    \
    --hook key:leftshift key:e key:g key:k key:y key:1 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:2 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:3 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:4 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:5 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:6 send-key=key:enter sequential \
    --withhold \
    \
    --hook key:leftctrl key:e toggle=leftctrl:2 send-key=key:leftshift send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    --withhold \
    --toggle key:leftctrl:1 key:leftctrl:1 key:leftctrl:0 id=leftctrl \
    --output @sparce @ctrl
# 
