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
    --output  name="keyboardB4Test" create-link=/dev/input/by-id/keyBoardB4Test repeat=disable 
sudo evsieve --input /dev/input/by-id/keyBoardB4Test grab domain=regular \
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
    --print format=direct \
    --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \
    # --map key:a key:a@F \ 
    # --hook key:leftctrl toggle=F \ 
    # --hook key:leftctrl:0 toggle=F \
    # --toggle @F @myF @regualrF id=F \
    # --map key:a@myF key:b \
    # --map key:a@regualrF key:leftctrl:0 key:c \
    # --map key:a:1@myF key:leftctrl:1 key:s:1 key:s:0 key:leftctrl:0 key:a:0 \
    # --map key:a:1@regualrF key:leftctrl:0 key:a \

sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null

# insert
# numlock
# caps
