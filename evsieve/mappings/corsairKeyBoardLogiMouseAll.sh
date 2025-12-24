evsieve --input $keyboardPath $mousePath grab domain=input \
--map key @keyboard \
--map btn:forward key:enter \
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "hi" "2"' \
--hook msc:scan:$codeForAppCodes toggle=appCodesToggle \
--map msc:scan:$codeForAppCodes @codeForAppCodesCount \
--toggle @codeForAppCodesCount @pressed1 @pressed2 @pressed3 id=appCodesToggle \
--hook @pressed3 toggle=appCodesToggle:2 \
--hook @pressed3 msc:scan:$codeForCode toggle=keyboardToggle:1 \
--hook @pressed3 msc:scan:$codeForGnomeTerminal toggle=keyboardToggle:2 \
--toggle @keyboard @keyboard$codeForCode @keyboard$codeForGnomeTerminal id=keyboardToggle \
`#--map key:a@keyboard$codeForCode key:b `\
`#--map key:a@keyboard$codeForGnomeTerminal key:c `\
--print @keyboard format=direct \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1




