#!/usr/bin/env bash
EVSIEVE_LOG_DIR="$(dirname "${BASH_SOURCE[0]}")/../log"    
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
## send keystrokes to test by pressing space:
# --map key:space:1 key:leftctrl:1 key:s:1 key:s:0 key:n:1 key:n:0 key:leftctrl:0 \
# systemd-run --service-type=notify --unit=keyBoardTest.service \
systemd-run --collect --service-type=notify --unit=keyBoardMouseTest.service \
evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse grab \
    --map msc:scan \
    --map led:numl \
    --output  name="keyBoardMouseB4Test" create-link=/dev/input/by-id/keyBoardMouseB4Test repeat=disable 

# coutn shift presses
# coutn control presses
sudo evsieve --input /dev/input/by-id/keyBoardMouseB4Test grab domain=regular \
    --map key:leftshift @leftShiftTemp \
    --hook @regular toggle=leftShiftCount:1 \
    --map key:leftshift @regular \
    --hook key:leftshift toggle=leftShiftCount \
    \
    --map key:leftctrl @leftControlTemp \
    --hook @regular toggle=leftControlCount:1 \
    --map key:leftctrl @regular \
    --hook key:leftctrl toggle=leftControlCount \
    --toggle key:leftshift @leftShiftMax @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount\
    --hook @leftShift3 toggle=leftShiftCount:3 \
    --toggle key:leftctrl @leftControlMax @leftControl1 @leftControl2 @leftControl3 id=leftControlCount\
    --hook @leftControl3 toggle=leftControlCount:3 \
    --print format=direct \
    --output create-link=/dev/input/by-id/keyBoardMouseTest repeat=disable > $EVSIEVE_LOG_DIR/evsieve.log.txt | grep -v 'Quit'

sudo systemctl stop keyBoardMouseTest.service 2>/dev/null
sudo systemctl reset-failed keyBoardMouseTest.service 2>/dev/null

# insert
# numlock
# caps
# notify-send "Current time" "$(date '+%H:%M:%S')"
# --map key:capslock key:backspace \?
# --hook "" exec-shell="echo Hello, world!" 
# --hook "" exec-shell='echo "$(date +"%Y-%m-%d %T")"' \
# --output create-link=/dev/input/by-id/keyBoardTest repeat=disable >> /tmp/evsieve.log.txt 

    # --map key:leftshift key:leftshift@leftShift \
    # --map key:f1 key:f1@F \
    # --map key:f2 key:f2@F \
    # --map key:f3 key:f3@F \
    # --map key:f4 key:f4@F \
    # --map key:f5 key:f5@F \
    # --map key:f6 key:f6@F \
    # --map key:f7 key:f7@F \
    # --map key:f8 key:f8@F \
    # --map key:f9 key:f9@F \
    # --map key:f10 key:f10@F \
    # --map key:f11 key:f11@F \
    # --map key:f12 key:f12@F \
    # --hook key:leftctrl toggle=F \
    # --hook key:leftctrl:0 toggle=F \
    # --toggle @F @myF @regualrFWithoutCtrl id=F \
    # --map key:f2@myF key:leftctrl key:s \
    # --map key:f2@regualrFWithoutCtrl key:leftctrl:0 key:f2 \
    # --map key:f3@myF key:leftctrl key:y \
    # --map key:f3@regualrFWithoutCtrl key:leftctrl:0 key:f3 \
    # --map key:f4@myF key:leftctrl key:z \
    # --map key:f4@regualrFWithoutCtrl key:leftctrl:0 key:f4 \
    # --map key:compose:1 key:leftctrl:1 key:slash:1 key:slash:0 key:leftctrl:0 key:compose:0 \
    # --hook @regular toggle=leftShiftCount:1 \
    # --hook key:leftshift@leftShift toggle=leftShiftCount \
    # --toggle @leftShift @leftShift0 @leftShift1 @leftShift2 @leftShift3 id=leftShiftCount \
    # --hook key:leftshift@leftShift3 toggle=leftShiftCount:3 \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:6 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c sequential \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:5 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:v sequential \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:4 \
    #     breaks-on=key::1 send-key=key:enter  sequential \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:3 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:x sequential \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:2 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:a send-key=key:c sequential \
    # --hook key:leftshift@leftShift3 key:k key:e key:y key:g key:1 \
    #     breaks-on=key::1 send-key=key:leftctrl@regular send-key=key:c send-key=key:f sequential \
    # --withhold \
    # --map @leftShiftUp key:leftshift:0 \
    # --map key @regular \
    # --hook "" exec-shell='echo "$(date +"%Y-%m-%d %T")"' \
    # --print format=direct \
