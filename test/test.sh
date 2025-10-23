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
    --map key:j key:k \
    --output create-link=/dev/input/by-id/keyBoardTest repeat=disable \

sudo systemctl stop keyBoardTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardTest.service 2>/dev/null

# insert
# numlock
# caps
