# Get script directory for relative paths
SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

# KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)

EVSIEVE_LOG_FILE="$(realpath "${SCRIPT_DIR}/../log/log.txt")"
SYSTEMD_LOG_FILE="$(realpath "${SCRIPT_DIR}/../log/error.txt")"
ECHO_LOG_FILE="$(realpath "${SCRIPT_DIR}/../log/echo.txt")"
SEND_KEYS="$(realpath "${SCRIPT_DIR}/../toggle/sendKeys")"
> "$SYSTEMD_LOG_FILE"
for arg in "$@"; do
    if [[ "$arg" == "reset" ]]; then
        > "$EVSIEVE_LOG_FILE"
        > "$ECHO_LOG_FILE"
    fi
done
SERVICE_INITIALIZED_CODE=200
SERVICE_INITIALIZED_CODE1=200
SERVICE_INITIALIZED_CODE2=201
SERVICE_INITIALIZED_CODE3=202
# command="source /home/yaniv/.bashrc && $SEND_KEYS 'numlock'"
command="$SEND_KEYS 'numlock' 'SYN_REPORT' 'keyA' 'SYN_REPORT'"
systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service\
    --property=StandardError=file:$SYSTEMD_LOG_FILE\
    --property=StandardOutput=append:$EVSIEVE_LOG_FILE\
    evsieve\
    --input /dev/input/by-id/$KEYBOARD_BY_ID /dev/input/$MOUSE_EVENT grab domain=input\
    `#dont print these events`\
    --map rel @dontPrint`#mouse move`\
    --map btn @dontPrint`#mouse click`\
    --map msc:scan:589825 @dontPrint`#scan code for mouse click`\
    --map msc:scan:458976 @dontPrint`#scan code for left control`\
    --map msc:scan:589830 @dontPrint`#scan code for mouse thumb`\
    --map msc:scan:458978 @dontPrint`#scan code for left alt`\
    --map msc:scan:458795 @dontPrint`#scan code for tab`\
    --map msc:scan:458796 @dontPrint`#scan code for space`\
    `#send event to see if service is / not initialized`\
    --hook "" send-key=key:a@null\
    --map key:a@null msc:scan:$SERVICE_INITIALIZED_CODE\
    --toggle msc:scan:$SERVICE_INITIALIZED_CODE @serviceUnInitialized @serviceIinitialized id=serviceInitializedToggle\
    --hook "" toggle=serviceInitializedToggle:2\
    `#led numlock initialization`\
    --hook @serviceUnInitialized led:numl breaks-on=""  send-key=key:a@null exec-shell="echo 'hi 1'"\
    --map key:a:1@null key:numlock:1@send key:numlock:0@send\
    --block key:a:0@null\
    `#test`\
    --hook @serviceUnInitialized breaks-on="" send-key=key:a@null exec-shell="echo 'hi 2'"\
    --map key:a:1@null key:numlock:1@send key:numlock:0@send\
    --block key:a:0@null\
    `#--hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi'"\
    --hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi2'"\
    --hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi3'"\
    --hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi4'"\
    --hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi5'"\
    --hook @serviceUnInitialized breaks-on="" exec-shell="echo 'hi6'"\
    --map key:a:1@null key:numlock:1@send key:numlock:0@send\
    --block key:a:0@null`\
    `#--hook @serviceUnInitialized exec-shell="$SEND_KEYS 'SYN_REPORT'" breaks-on=""`\
    `#--hook @serviceUnInitialized led:numl send-key=key:a@null breaks-on=""`\
    `#--map key:a:1@null key:numlock:1@send key:numlock:0@send\
    --block key:a:0@null`\
    `#set numlock on after initialization`\
    `#test release in toggle`\
    --print key msc:scan:~199 msc:scan:201~589824 format=direct\
    `#--print @input @null @unInitialized format=direct`\
    `#--output @send @input @myOutput @unsievedOutput @dontPrint @setNumLockOnAfterInitialization\
    name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable`\
    --output name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable\
    `# mouse events`\
    --map btn:forward key:enter\
    `#--print key format=direct`


