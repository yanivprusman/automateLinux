# exec 2>/dev/null
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/keyBoard grab \
    --map msc:scan \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \
    # --hook "" exec-shell="echo Hello, world!" \
    # --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \

sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
    --map key:leftshift key:leftshift@leftShift \
    --toggle key:leftshift@leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 \
    --map key:leftshift @leftShift \
    --hook @regular toggle=leftShiftCount:1 \
    --hook key:leftshift@leftShift exec-shell="echo Hello, world!" \
    --hook key:leftshift@leftShift toggle=leftShiftCount \
    --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount \
    --hook key:leftshift@leftShift3 \
        toggle=leftShiftCount:3 \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:6 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:5 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:v sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:4 \
        breaks-on=key::1 send-key=key:enter  sequential \
    --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:3 \
        breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:x sequential \
    --withhold \
    --map led:numl @numl \
    --map @leftShiftUp key:leftshift:0 \
    --map key @regular \
    --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \

    # --hook "" send-key=key:leftshift@theLeft \
    # --toggle @theLeft @theLeft0 @theLeft1 @theLeft2 @theLeft3 \


    # --print format=direct \
    # --map key:leftshift @leftShiftLoaded \
    # --toggle @leftShiftLoaded @leftShift @leftShiftLoaded id=leftShiftLoaded \
    # --hook @regular toggle=leftShiftCount:1 toggle=leftShiftLoaded:1\
    # --hook @leftShift toggle=leftShiftCount \
    # --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount \
    # --hook @leftShift3 \
    #     toggle=leftShiftLoaded:2 send-key=left\
    # --hook @leftShiftLoaded key:k key:e key:y key:g key:6 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:5 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:v sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:4 \
    #     breaks-on=key::1 send-key=key:enter  sequential \
    # --hook key:leftshift key:k key:e key:y key:g key:3 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:x sequential \
    # --withhold \
    # --map led:numl @numl \
    # --map @leftShiftUp key:leftshift:0 \
    # --map key @regular \
    # --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \

    # --hook key:leftctrl@regular key:s \
    #     send-key=key:leftctrl@output send-key=key:n sequential \
    # --withhold key:s \
    # --map key:leftctrl:0@output \
    # \
    # --withhold key:k key:e key:y key:g key:1 key:2 key:3 key:4 key:5 key:6 \ propblem shift+e != E
    # --withhold \ propblem g6 opens terminal (ctrl shift c)

    # --print format=direct \
    # --hook key:leftctrl@copy key:e \
    #     breaks-on=key::1 send-key=key:leftshift send-key=key:a send-key=key:s@copy send-key=key:d send-key=key:f sequential \
    # --hook key:leftctrl@copy key:r \
    #     breaks-on=key::1 send-key=key:leftshift  send-key=key:s@copy send-key=key:d send-key=key:f send-key=key:g sequential \
    # --hook key:leftctrl@copy key:s@regular \
    #     send-key=key:leftctrl@regular send-key=key:n sequential \
    # --withhold \
    # --hook key:leftctrl@copy key:s@regular \
    #     \
    # --withhold \

sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null

