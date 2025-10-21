# exec 2>/dev/null
sudo evsieve --input /dev/input/by-id/keyBoard grab \
    --hook key:leftshift key:k key:e key:y key:g key:6 \
        send-key=key:leftctrl send-key=key:c sequential \
    --print format=direct \
    --hook key:leftshift key:k key:e key:y key:g key:5 \
        send-key=key:leftctrl send-key=key:v sequential \
    --withhold \
    \
    --toggle key:leftctrl:1 key:leftctrl:1 key:leftctrl:0 id=leftctrl \
    --toggle @regular @regular @devNull id=regularToDevNull \
    --output 
# 
    # --hook key:leftctrl key:e toggle=leftctrl:2 send-key=key:leftshift send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    # --withhold \
    # --print format=direct \
    # --withhold \
    # --withhold \
    # --hook key:leftshift key:k key:e key:y key:g key:4 send-key=key:enter sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:3 send-key=key:leftctrl send-key=key:x sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:2 send-key=key:leftctrl send-key=key:a send-key=key:c sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:1 send-key=key:leftctrl send-key=key:f sequential \
# ssdfg sdfg sddddd sddddd sdfg sdd6keyg key:g key:gdd cDF ASDFggggg

# 6 copy                KEYG^   6keyg
# 5 paste               KEYG% 
# 4 enter               KEYG$   4keyg
# 3 cut                 KEYG#   3keyg
# 2 select all and copy KEYG@   2keyg
# 1 copy and search     KEYG!   1keyg


