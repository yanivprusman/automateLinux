sudo evsieve --input /dev/input/by-id/usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd grab domain=input \
`#--map btn:forward key:enter`\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1

sudo evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse grab domain=input \
`#--map btn:forward key:enter`\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse1 2>&1

sudo evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse grab domain=input \
`#--map btn:forward key:enter`\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse2 2>&1

sudo evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse domain=input \
--map key:1 key:2 \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse1 2>&1

sudo evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse domain=input \
`#--map btn:forward key:enter`\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse2 2>&1

sudo evsieve --input /dev/input/by-id/corsairKeyBoardLogiMouse1 domain=input \
--map key:1 key:2 \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse2 2>&1

sudo evsieve --input /dev/input/by-id/usb-Corsair_CORSAIR_K100_RGB_Optical-Mechanical_Gaming_Keyboard_502A81D24AAA7CC6-event-kbd grab domain=input \
`#--map btn:forward key:enter`\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1
