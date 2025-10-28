KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
EVSIEVE_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/log.txt")
SYSTEMD_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/error.txt")
ECHO_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/echo.txt")
> "$SYSTEMD_LOG_FILE"
for arg in "$@"; do
    if [[ "$arg" == "reset" ]]; then
        > "$EVSIEVE_LOG_FILE"
    fi
done
systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service\
    --property=StandardError=file:$SYSTEMD_LOG_FILE\
    --property=StandardOutput=append:$EVSIEVE_LOG_FILE\
    evsieve\
    --input /dev/input/by-id/$KEYBOARD_BY_ID /dev/input/$MOUSE_EVENT grab domain=input\
    `#dont print these events`\
    --map rel @dontPrint\
    `#led numlock initialization`\
    --toggle led:numl @unInitialized @initialized id=ledInitialized\
    --hook "" toggle=ledInitialized:2\
    --hook @unInitialized exec-shell="echo 'initialized led' > $ECHO_LOG_FILE"\
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
    --print key format=direct\
    --output @input @myOutput @unsievedOutput @dontPrint name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable\
    `# mouse events`\
    --map btn:forward key:enter\
    `# toggle outputs`\
    `# toggle outputs`\
    `# --print key format=direct`\


