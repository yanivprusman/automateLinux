evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--hook key:leftctrl key:1 exec-shell='notify-send "Title" "Your message here"' \
--hook key:grave exec-shell="echo asdf > /tmp/test.txt" \
--hook key:leftctrl key:1 exec-shell="echo $randomVar > /tmp/test.txt" \
`#--hook key:grave toggle=test  `\
--hook key:leftctrl key:h exec-shell="echo Hello, world!" \
--toggle key:2 key:3 key:4 id=test \
--map key:1 key:5 \
--output 2>&1
