# exec 2>/dev/null
sudo evsieve --input /dev/input/by-id/keyBoard grab domain=regular\
    --hook "" toggle=regularToDevNull:1 \
    --map key:a:1 key:a:1 \
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
        breaks-on=key::1 toggle=regularToDevNull:2 send-key=key:leftshift@special send-key=key:a@special send-key=key:s@special send-key=key:d@special send-key=key:f@special sequential \
    --hook key:leftctrl key:r \
        breaks-on=key::1 toggle=regularToDevNull:2 send-key=key:leftshift@special  send-key=key:s@special send-key=key:d@special send-key=key:f@special send-key=key:g@special sequential \
    --withhold \
    --hook key:leftctrl@regular key:s \
        breaks-on=key::1 toggle=regularToDevNull:2 send-key=key:leftctrl@special send-key=key:n@special sequential \
    --withhold \
    --toggle @regular @regular @devNull id=regularToDevNull\
    --output @regular @special repeat=disable \


    # --withhold \
# 
    # --print format=direct \
    # --hook key:leftshift key:k key:e key:y ke
    # --hook key:leftshift key:k key:e ke:e ke

# 6 copy   
# 5 paste               
# 4 enter               
# 3 cut                 
# 2 select all and copy 
# 1 copy and search     
# 
