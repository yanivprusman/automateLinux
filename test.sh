# exec 2>/dev/null
sudo evsieve --input /dev/input/by-id/keyBoard grab \
    --hook key:leftshift key:k key:e key:y key:g key:6 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:c sequential \
    --hook key:leftshift key:k key:e key:y key:g key:5 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:v sequential \
    --hook key:leftshift key:k key:e key:y key:g key:4 \
        breaks-on=key::1 send-key=key:enter  sequential \
    --hook key:leftshift key:k key:e key:y key:g key:3 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:x sequential \
    --withhold \
    --hook key:leftctrl key:e \
        breaks-on=key::1 send-key=key:leftshift send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    --withhold \
    --output repeat=disable \


    # --withhold \
# 
    # --print format=direct \
    # --hook key:leftshift key:k key:e key:y key:g key:2 send-key=key:leftctrl send-key=key:a send-key=key:c sequential \
    # --hook key:leftshift key:k key:e ke:e ke

# 6 copy   
# 5 paste               
# 4 enter               
# 3 cut                 
# 2 select all and copy 
# 1 copy and search     
# 
