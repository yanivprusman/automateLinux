evsieve --input $keyboardPath  grab domain=keyboardInput \
--input $mousePath grab domain=mouseInput \
--map btn:forward key:enter \
--hook key:leftctrl key:grave exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "terminal"' \
`#--hook key:grave exec-shell="sudo -u yaniv sh -c 'whoami > /tmp/test2.txt'" `\
--output 2>&1
