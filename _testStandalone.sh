# exec 2>/dev/null
KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# sudo evsieve --input /dev/input/by-id/evsieveKeyboardPhase2 domain=evsieveKeyboardPhase2 grab \
sudo evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab \
    --hook key:leftshift key:e key:g key:k key:y key:6 send-key=key:leftctrl send-key=key:c sequential \
    --hook key:leftshift key:e key:g key:k key:y key:5 send-key=key:leftctrl send-key=key:v sequential \
    --hook key:leftshift key:e key:g key:k key:y key:4 send-key=key:enter sequential \
    --hook key:leftshift key:e key:g key:k key:y key:3 send-key=key:leftctrl send-key=key:x sequential \
    --hook key:leftshift key:e key:g key:k key:y key:2 send-key=key:leftctrl send-key=key:a send-key=key:c sequential \
    --hook key:leftshift key:e key:g key:k key:y key:1 send-key=key:leftctrl send-key=key:f sequential \
    --withhold \
    \
    --hook key:leftctrl key:e toggle=leftctrl:2 send-key=key:leftshift send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    --withhold \
    --toggle key:leftctrl:1 key:leftctrl:1 key:leftctrl:0 id=leftctrl \
    --print format=direct\
    --output 
# 
