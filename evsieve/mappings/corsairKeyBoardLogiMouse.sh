KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
EVSIEVE_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/log.txt")
SYSTEMD_LOG_FILE=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../log/error.txt")
> "$SYSTEMD_LOG_FILE"
for arg in "$@"; do
    if [[ "$arg" == "reset" ]]; then
        > "$EVSIEVE_LOG_FILE"
    fi
done
systemd-run --collect --service-type=notify --unit=corsairKeyBoardLogiMouse.service \
    --property=StandardError=file:$SYSTEMD_LOG_FILE \
    --property=StandardOutput=append:$EVSIEVE_LOG_FILE \
    evsieve \
    --input /dev/input/by-id/$KEYBOARD_BY_ID /dev/input/$MOUSE_EVENT grab domain=input \
    `# toggle outputs` \
    --copy @input @unsievedOutput \
    --map @input @myOutput \
    --hook key:numlock toggle=myOutputNull toggle=unsievedOutputNull \
    --map key:numlock @dontPrint \
    --toggle @myOutput @myOutput @null id=myOutputNull \
    --toggle @unsievedOutput @null @unsievedOutput id=unsievedOutputNull \
    `# mouse events` \
    --map btn:forward key:enter \
    `# keyboard events` \
    --print key@myOutput key@unsievedOutput format=direct \
    --output @myOutput @unsievedOutput @dontPrint name="combined corsair keyboard and logi mouse" create-link=/dev/input/by-id/corsairKeyBoardLogiMouse repeat=disable

