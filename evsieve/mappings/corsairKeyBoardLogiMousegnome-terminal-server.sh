evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
`#--hook key:leftctrl key:1 exec-shell='notify-send "Title" "Your message here"' `\
`#--hook key:leftctrl key:1 exec-shell="echo asdf" `\
--hook key:leftctrl key:1 exec-shell="echo asdf > ~/coding/automateLinux/terminal/test.txt" \
--hook key:leftctrl key:h exec-shell="echo Hello, world!" \
--map key:1 key:5 \
--output 2>&1

1