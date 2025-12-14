evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "Code"' \
`#--hook led:numl:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "Code"' `\
`#--hook led:numl:1 exec-shell='sudo -u yaniv sendKeys keyA backspace' `\
`#                          'theRealPath' `\
`#                          'pwd' returns \ `\
`#                          'sudo theRealPath' `\
`#                          'echo -n ' `\
`#                          'daemon' `\
`#                          'print' `\
--hook btn:left exec-shell='echo -n' \
`#--hook btn:g exec-shell='echo asdf' `\
--output name=corsairKeyBoardLogiMouse 2>&1



