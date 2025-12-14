#echo asdf
auso evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "Code"' \
`#--hook led:numl:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "Code"' `\
`#--hook led:numl:1 exec-shell='sudo -u yaniv sendKeys keyA backspace' `\
`#--hook btn:left exec-shell='sudo -u yaniv d' `\
--output name=corsairKeyBoardLogiMouse 2>&1

