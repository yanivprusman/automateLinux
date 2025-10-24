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
    # --toggle @F @myF @regualrFWithoutCtrl id=F \
    # --print format=direct \
    # --map key:f2:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:f2:0 \
# systemd-run --service-type=notify --unit=corsairKeyBoard.service \
systemd-run --collect --service-type=notify --unit=corsairKeyBoard.service \
    evsieve --input /dev/input/by-id/$KEYBOARD_BY_ID grab domain=regular \
    --output name="corsair keyboard" create-link=/dev/input/by-id/corsairKeyBoard repeat=disable 


