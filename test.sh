# exec 2>/dev/null
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/keyBoard grab \
    --map msc:scan \
    --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \
    # --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \ not working
    # --map key:space:1 key:leftctrl:1 key:n:1 key:n:0 key:s:1 key:s:0 key:leftctrl:0 \ working
    # --map key:space:1 key:leftctrl:1 key:n:1 key:n:0 key:leftctrl:0 \                 working
    # --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 \                 working


sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
    \--map key:leftshift @leftShift \
    --hook @regular toggle=leftshiftCount:1 \
    --hook @leftShift toggle=leftshiftCount \
    --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftshiftCount \
    --hook @leftShift3 \
        toggle=leftshiftCount send-key=key:leftshift send-key=key:a send-key=key:s send-key=key:d send-key=key:f sequential \
    --map key @regular \
    --output @regular create-link=/dev/input/by-id/keyBoardTest repeat=disable \
    # --copy key:leftshift key:leftshift@copy \
    # --copy key:leftctrl key:leftctrl@copy \
    # --hook key:leftshift@copy key:k key:e key:y key:g key:6 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c sequential \
    # --hook key:leftshift@copy key:k key:e key:y key:g key:5 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:v sequential \
    # --hook key:leftshift@copy key:k key:e key:y key:g key:4 \
    #     breaks-on=key::1 send-key=key:enter  sequential \
    # --hook key:leftshift@copy key:k key:e key:y key:g key:3 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:x sequential \
    # --withhold \
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

