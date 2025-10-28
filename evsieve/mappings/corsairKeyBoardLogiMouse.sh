KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
EVSIEVE_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/log.txt")
SYSTEMD_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/error.txt")
ECHO_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/echo.txt")
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
    `#--hook @serviceUnInitialized led:numl send-key=key:numlock@send breaks-on=@serviceIinitialized`\
    --hook @serviceUnInitialized led:numl send-key=key:a@null breaks-on=@serviceIinitialized\
    --map key:a:1@null key:numlock:1@send key:numlock:0@send\
    --block key:a:0@null\
    `#numlock toggle logic`\
    --print key:numlock format=direct\
    `#led numlock toggle logic`\
    `#--toggle led:numl @ledNumUnInitialized @ledNumlInitialized id=ledNumlInitializedToggle\
    --hook "" toggle=ledNumlInitializedToggle:2\
    --hook @ledNumUnInitialized send-key=key:numlock@numlockInitialize\
    --map @numlockInitialize key:numlock:1@ledNumlInitialized key:numlock:0@ledNumlInitialized`\
    `#--copy @serviceUnInitialized key:numlock@setNumLockOnAfterInitialization %4:%4:%28`\
    `#--map key:a@null key:a@null`\
    `#--hook msc:scan:$SERVICE_INITIALIZED_CODE@serviceUnInitialized led:numl send-key=key:numlock@numlockInitialize sequential breaks-on=@serviceIinitialized\
    --map @numlockInitialize key:numlock:1@numLockInitialized key:numlock:0@numLockInitialized\
    --print led key:numlock format=direct`\
    `#--print msc:scan:$SERVICE_INITIALIZED_CODE led format=direct`\
    `#--map @numlockUp key:numlock:1 key:numlock:0 key:numlock:1 key:numlock:0`\
    `#--hook`\
    `#--print led key:numlock format=direct`\
    `#--hook led:numl:1 key:numlock:0 send-key=key:f`\
    `#--print @input format=direct`\
    `#--map key:a key:numlock`\
    `# toggle outputs`\
    `#--hook led:numl:0 key:numlock send-key=key:numlock`\
    `#--copy @input @unsievedOutput`\
    `#--map @input @myOutput`\
    `#--hook key:numlock toggle=myOutputNull toggle=unsievedOutputNull`\
    `# --map key:numlock @dontPrint`\
    `#--toggle @myOutput @myOutput @null id=myOutputNull`\
    `#--toggle @unsievedOutput @null @unsievedOutput id=unsievedOutputNull`\
    `# keyboard events`\
    `#-print @input @null @unInitialized format=direct`\
    --output @send @input @myOutput @unsievedOutput @dontPrint @setNumLockOnAfterInitialization \
    name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable\
    `# mouse events`\
    --map btn:forward key:enter\
    `# toggle outputs`\
    `# toggle outputs`\
    --print key format=direct


