evsieve --input $keyboardPath $mousePath grab domain=input \
--map key @keyboard                                              `#part of detecting window`\
--map btn:forward key:enter `#mouse to enter`\
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "hi" "5"'`#sanity check` \
--copy key @gprogress \
--hook msc:scan:$codeForAppCodes toggle=appCodesToggle         `#detecting window continued`\
--map msc:scan:$codeForAppCodes @codeForAppCodesCount                                       \
--toggle @codeForAppCodesCount @pressed1 @pressed2 @pressed3 id=appCodesToggle              \
--hook @pressed3 toggle=appCodesToggle:2                                                    \
--hook @pressed3 msc:scan:$codeForCode toggle=keyboardToggle:1                              \
--hook @pressed3 msc:scan:$codeForGnomeTerminal toggle=keyboardToggle:2                     \
--toggle @keyboard @keyboard$codeForCode @keyboard$codeForGnomeTerminal id=keyboardToggle   \
`#                                                                       detecting G keys  `\
`#                             1       2      3       4     5      6                       `\
`#                          ctrl10  shift10 ctrl10 shift10 N10 backspace10                 `\
--hook key:leftctrl@gprogress1 toggle=GToggle \
--hook @gprogress2 toggle=GToggle:1 \
--hook key:leftshift@gprogress2 toggle=GToggle:3 \
--hook @gprogress3 toggle=GToggle:1 \
--hook key:leftctrl@gprogress3 toggle=GToggle:4 \
--hook @gprogress4 toggle=GToggle:1 \
--hook key:leftshift@gprogress4 toggle=GToggle:5 \
`#--hook key`\
--toggle @gprogress @gprogress1 @gprogress2 @gprogress3 @gprogress4 @gprogress5 @gprogress6 id=GToggle \
--toggle @gKey @gKey1 @gKey2 @gKey3 @gKey4 @gKey5 @gKey6 \
--print format=direct \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1




