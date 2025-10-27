MOUSE_EVENT=$(awk '/Logitech/ && /Mouse/ {found=1} found && /Handlers/ {if (match($0, /event[0-9]+/, a)) {print a[0]; exit}}' /proc/bus/input/devices)
# echo "Detected mouse: /dev/input/$MOUSE_EVENT"
systemd-run --service-type=notify --unit=logiMouse.service \
    evsieve --input /dev/input/$MOUSE_EVENT grab \
    --map btn:forward key:enter \
    --output name="evsieve mouse" create-link=/dev/input/by-id/mouse


