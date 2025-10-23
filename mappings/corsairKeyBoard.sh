KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
# f2        save
# f3        redo
# f4        undo
# compopse  comment
# G6         copy   
# G5         paste               
# G4         enter               
# G3         cut                 
# G2         select all and copy 
# G1         copy and search    
    # --map key:f2 key:f2@F \
    # --hook key:leftctrl key:f2 toggle=F \
    # --toggle @F @myF @regualrF id=F \
    # --print format=direct \
    # --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:f2:0 \
# systemd-run --service-type=notify --unit=corsairKeyBoard.service \
systemd-run --collect --service-type=notify --unit=corsairKeyBoard.service \
    evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab domain=regular \
    --map key:f2 key:f2@F \
    --map key:f3 key:f3@F \
    --map key:f4 key:f4@F \
    --hook key:leftctrl toggle=F \
    --hook key:leftctrl:0 toggle=F \
    --toggle @F @myF @regualrF id=F \
    --map key:f2@myF key:leftctrl key:s \
    --map key:f2@regualrF key:leftctrl:0 key:f2 \
    --map key:f3@myF key:leftctrl key:y \
    --map key:f3@regualrF key:leftctrl:0 key:f3 \
    --map key:f4@myF key:leftctrl key:z \
    --map key:f4@regualrF key:leftctrl:0 key:f4 \
    --map key:compose:1 key:leftctrl:1 key:slash:1 key:slash:0 key:leftctrl:0 key:compose:0 \
    --map key:leftshift key:leftshift@leftShift \
    --toggle key:leftshift@leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 \
    --map key:leftshift @leftShift \
    --hook @regular toggle=leftShiftCount:1 \
    --hook key:leftshift@leftShift toggle=leftShiftCount \
    --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount \
    --hook key:leftshift@leftShift3 toggle=leftShiftCount:3 \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:6 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:5 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:v sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:4 \
        breaks-on=key::1 send-key=key:enter  sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:3 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:x sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:2 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:a send-key=key:c sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:1 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c send-key=key:f sequential \
    --withhold \
    --map @leftShiftUp key:leftshift:0 \
    --map key @regular \
    --output name="corsair keyboard" create-link=/dev/input/by-id/corsairKeyBoard repeat=disable 


