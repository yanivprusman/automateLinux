evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "hi" "2"' \
--toggle msc:scan:$codeForAppCodes @pressed1 @pressed2 id=appCodesToggle \
--toggle key:a @pressed1 @pressed2 id=aToggle \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1




