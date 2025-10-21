# exec 2>/dev/null
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/keyBoard grab domain=regular \
    --map key:space:1 key:leftctrl:1 key:n:1 key:n:0 key:s:1 key:s:0 key:leftctrl:0 \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \
    # --map key:space:1 key:leftctrl:1 key:n:1 key:n:0 key:s:1 key:s:0 key:leftctrl:0 \
    # --map key:space:1 key:leftctrl:1 key:n:1 key:n:0 key:leftctrl:0 \
    # --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 \ the only one that is working


sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
    --hook "" toggle=regularToDevNull:1 --map key:a:1 key:a:1 \
    --map msc:scan \
    --hook key:leftshift key:k key:e key:y key:g key:6 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:c sequential \
    --hook key:leftshift key:k key:e key:y key:g key:5 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:v sequential \
    --hook key:leftshift key:k key:e key:y key:g key:4 \
        breaks-on=key::1 send-key=key:enter  sequential \
    --hook key:leftshift key:k key:e key:y key:g key:3 \
        breaks-on=key::1 send-key=key:leftctrl send-key=key:x sequential \
    --withhold \
    --hook key:leftctrl@regular key:e \
        breaks-on=key::1 toggle=regularToDevNull:2 send-key=key:leftshift@special send-key=key:a@special send-key=key:s@special send-key=key:d@special send-key=key:f@special sequential \
    --hook key:leftctrl@regular key:r \
        breaks-on=key::1 toggle=regularToDevNull:2 send-key=key:leftshift@special  send-key=key:s@special send-key=key:d@special send-key=key:f@special send-key=key:g@special sequential \
    --withhold \
    --copy key:leftctrl key:leftctrl@copy \
    --hook key:leftctrl@copy key:s \
        toggle=regularToDevNull:2 send-key=key:leftctrl@special send-key=key:n@special sequential \
    --withhold \
    --hook key:leftctrl@copy key:s \
        \
    --withhold \
    --toggle @regular @regular @devNull id=regularToDevNull\
    --print format=direct \
    --output @regular @special create-link=/dev/input/by-id/keyBoardTest repeat=disable \

sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null

