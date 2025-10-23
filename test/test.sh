# exec 2>/dev/null
# useful stuff:
## print:
# --print format=direct \
## avoid clutter:
# --map led:numl @numl \ 
## switch ctrl+s -> ctrl+n:
# --hook key:leftctrl@regular key:s \
#     send-key=key:leftctrl@output send-key=key:n sequential \
# --withhold key:s \
# --map key:leftctrl:0@output \
## echo:
# --hook "" exec-shell="echo Hello, world!" \

# send keystrokes to test by pressing space:
# --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/corsairKeyBoard grab \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \
# mappings:
# g6 - copy
# g5 - paste
# g4 - enter
# g3 - cut 
# g2 - select all and copy
# g1 - copy and search
sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
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
    --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \

sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null

