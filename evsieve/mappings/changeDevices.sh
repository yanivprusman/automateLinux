read -r -d '' terminal <<EOF
sudo evsieve --input $keyboardPath  grab domain=keyboardInput \
--input $mousePath grab domain=mouseInput \
--map btn:forward key:enter \
--hook key:leftctrl key:grave exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "terminal"' \
--hook key:grave exec-shell='sudo -u yaniv sendKeys keyA 2>&1 ' \
--output name=corsairKeyBoardLogiMouse 2>&1
EOF

read -r -d '' chrome <<EOF
sudo evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--hook key:leftctrl key:grave exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "Keyboard" "google-chrome"' \
--hook key:grave exec-shell='sudo -u yaniv sendKeys keyA keyB backspace backspace 2>&1 ' \
--map key:grave \
--output name=corsairKeyBoardLogiMouse 2>&1
EOF
