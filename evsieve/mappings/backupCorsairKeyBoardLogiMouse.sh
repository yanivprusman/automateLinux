# #!/bin/bash
# # Get script directory for relative paths
# # SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
# # KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')

# MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
# EVSIEVE_LOG_FILE="$(theRealPath ../log/log.txt)"
# SYSTEMD_LOG_FILE="$(theRealPath ../log/error.txt)"
# ECHO_LOG_FILE="$(theRealPath ../log/echo.txt)"
# EVSIEVE_LOG_FILE2="$(theRealPath ../log/log2.txt)"
# SYSTEMD_LOG_FILE2="$(theRealPath ../log/error2.txt)"
# ECHO_LOG_FILE2="$(theRealPath ../log/echo2.txt)"
# SEND_KEYS="$(theRealPath ../../utilities/sendKeys)"
# # Initialize log files
# DELETE_LOGS="$(theRealPath ../log/deleteLogFiles.sh)"

# # Handle reset argument
# for arg in "$@"; do
#     if [[ "$arg" == "reset" ]]; then
#         "$DELETE_LOGS" --reset
#     fi
# done
# SERVICE_UNINITIALIZED_CODE=200
# SERVICE_INITIALIZED_CODE=201
# # command="source /home/yaniv/.bashrc && $SEND_KEYS 'numlock'"
# command="$SEND_KEYS 'numlock' 'SYN_REPORT' 'keyA' 'SYN_REPORT'"
# systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse1.service \
#     --property=StandardError=file:$SYSTEMD_LOG_FILE \
#     --property=StandardOutput=append:$EVSIEVE_LOG_FILE \
#     evsieve \
#     --input /dev/input/by-id/$KEYBOARD_BY_ID /dev/input/$MOUSE_EVENT grab domain=input \
#     `#dont print these events`\
#     --copy "" @copyForPrint\
#     --map rel@copyForPrint @dontPrint`#mouse move`\
#     --map btn@copyForPrint @dontPrint`#mouse click`\
#     --map key:leftctrl@copyForPrint @dontPrint\
#     --map key:1@copyForPrint @dontPrint\
#     --map msc:scan:589825@copyForPrint @dontPrint`#scan code for mouse click`\
#     --map msc:scan:458976@copyForPrint @dontPrint`#scan code for left control`\
#     --map msc:scan:589830@copyForPrint @dontPrint`#scan code for mouse thumb`\
#     --map msc:scan:458978@copyForPrint @dontPrint`#scan code for left alt`\
#     --map msc:scan:458795@copyForPrint @dontPrint`#scan code for tab`\
#     --map msc:scan:458796@copyForPrint @dontPrint`#scan code for space`\
#     --map msc:scan:458782@copyForPrint @dontPrint`#scan code for 1`\
#     --map msc:scan:~199@copyForPrint @dontPrint\
#     --map msc:scan:201~589824@copyForPrint @dontPrint\
#     --map key:right@copyForPrint @dontPrint\
#     --map key:up@copyForPrint @dontPrint\
#     --map key:left@copyForPrint @dontPrint\
#     --map key:down@copyForPrint @dontPrint\
#     --map key:tab@copyForPrint @dontPrint\
#     --map key:leftalt@copyForPrint @dontPrint\
#     --block @dontPrint\
#     --hook key:leftctrl key:1 exec-shell="$DELETE_LOGS --reset" \
#     --print @copyForPrint format=direct \
#     --block @copyForPrint\
#     --output name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse1 repeat=disable

# systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse2.service\
#     --property=StandardError=file:$SYSTEMD_LOG_FILE2\
#     --property=StandardOutput=append:$EVSIEVE_LOG_FILE2\
#     evsieve\
#     --input /dev/input/by-id/corsairKeyBoardLogiMouse1 grab domain=input\
#     `#send event to see if service is / not initialized`\
#     --hook "" send-key=key:a@null\
#     --map key:a@null msc:scan:$SERVICE_UNINITIALIZED_CODE\
#     --toggle msc:scan:$SERVICE_UNINITIALIZED_CODE msc:scan:$SERVICE_UNINITIALIZED_CODE@serviceUnInitialized msc:scan:$SERVICE_INITIALIZED_CODE@serviceIinitialized id=serviceInitializedToggle\
#     --hook "" toggle=serviceInitializedToggle:2\
#     `#--print msc key led format=direct`\
#     `#--copy led:numl:0 @numLedOff`\
#     `#--copy led:numl:1 @numLedOn`\
#     --hook msc:scan:$SERVICE_UNINITIALIZED_CODE@serviceUnInitialized send-key=key:numlock@initState\
#     `#--hook @numLedOn send-key=key:numlock@initState`\
#     `#--block key:numlock@initState`\
#     --print key msc:scan:~199 msc:scan:201~589824 format=direct\
#     --output name="combined2 corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse2 repeat=disable\
#     --map btn:forward key:enter


systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service \
    evsieve \
    --input /dev/input/by-id/$KEYBOARD_BY_ID grab domain=input \
    --output

sudo evsieve \
--input /dev/input/by-id/$KEYBOARD_BY_ID grab domain=input \
--output

evsievep | grep --line-buffered -E 'msc|key:n' | grep --line-buffered -v 'msc:scan:589825'
evsievep | grep --line-buffered -E 'event23|event23' | grep --line-buffered -v 'msc:scan:589825'

led/numlock from off to on:
msc:scan:458835@/dev/input/event23
key:numlock:1@/dev/input/event23
led:numl:1@/dev/input/event23
msc:scan:458835@/dev/input/event23
key:numlock:0@/dev/input/event23

led/numlock from on to off:
msc:scan:458835@/dev/input/event23
key:numlock:1@/dev/input/event23
msc:scan:458835@/dev/input/event23
key:numlock:0@/dev/input/event23
led:numl:0@/dev/input/event23
