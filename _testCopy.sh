# exec 2>/dev/null
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/keyBoard grab \
    --map msc:scan \
    --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \

sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
    --map key:leftshift @leftShift \
    --hook @regular toggle=leftShiftCount:1 \
    --hook @leftShift toggle=leftShiftCount \
    --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount \
    --hook @leftShift3 \
        toggle=leftShiftCount:1 send-key=key:leftshift@leftShiftUp send-key=key:a send-key=key:s send-key=key:d send-key=key:f \
    --map @leftShiftUp key:leftshift:0 \
    --map key @regular \
    \
    --hook key:leftctrl@regular key:s \
        send-key=key:leftctrl@output send-key=key:n sequential \
    --withhold key:s \
    --map key:leftctrl:0@output \
    --map led:numl @numl \
    --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \
