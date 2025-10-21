# exec 2>/dev/null
# systemd-run --collect --service-type=notify --unit=keyBoardTest.service \
systemd-run --service-type=notify --unit=keyBoardTest.service \
evsieve --input /dev/input/by-id/keyBoard grab domain=regular \
    --hook key:g \
        send-key=key:h \
    --withhold \
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable \


        # send-key=key:leftctrl send-key=key:n:1 send-key=key:n:0 send-key=key:s:1 send-key=key:s:0 send-key=key:leftctrl \
    # --map key:rightctrl@regular key:rightctrl@special \
    # --map key:backslash@regular key:backslash@special \
    # --map key:enter@regular key:enter@special \
    # --map key@regular \
