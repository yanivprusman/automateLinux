evsieve --input $keyboardPath $mousePath grab domain=input \
--map key @keyboard                                              `#part of detecting window`\
`#`\
--map btn:forward key:enter                                                `#mouse to enter`\
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "hi" "5"'`#sanity check` \
--copy key @gprogress                                            `#part of detecting G keys`\
                                                               `#detecting window continued`\
--toggle @gprogress @gprogress1 @gprogress2 @gprogress3 @gprogress4 @gprogress5 id=GToggle  \
--hook msc:scan:$codeForAppCodes toggle=appCodesToggle         \
--map msc:scan:$codeForAppCodes @codeForAppCodesCount                                       \
--toggle @codeForAppCodesCount @pressed1 @pressed2 @pressed3 id=appCodesToggle              \
--hook @pressed3 toggle=appCodesToggle:2                                                    \
--hook @pressed3 msc:scan:$codeForCode toggle=keyboardToggle:1                              \
--hook @pressed3 msc:scan:$codeForGnomeTerminal toggle=keyboardToggle:2                     \
--hook @pressed3 msc:scan:$codeForGoogleChrome toggle=keyboardToggle:3                            \
--toggle @keyboard @keyboard$codeForCode @keyboard$codeForGnomeTerminal @keyboard$codeForGoogleChrome id=keyboardToggle   \
`#         detecting G keys continued             `\
`#     1       2      3       4     5      6      `\
`#  ctrl10  shift10 ctrl10 shift10 N10 backspace10`\
--hook key:leftctrl@gprogress1 toggle=GToggle      \
--hook @gprogress2 toggle=GToggle:1                \
--hook key:leftshift@gprogress2 toggle=GToggle:3   \
--hook @gprogress3 toggle=GToggle:1                \
--hook key:leftctrl@gprogress3 toggle=GToggle:4    \
--hook @gprogress4 toggle=GToggle:1                \
--hook key:leftshift@gprogress4 toggle=GToggle:5   \
--hook @gprogress5 toggle=GToggle:1                \
--print @nothing                                   \
--hook key:1 key:1@gprogress5 exec-shell='echo G1' \
--withhold                                         \
--hook key:2 key:2@gprogress5 exec-shell='echo G2' \
--withhold                                         \
--hook key:3 key:3@gprogress5 exec-shell='echo G3' \
--withhold                                         \
--hook key:4 key:4@gprogress5 exec-shell='echo G4' \
--withhold                                         \
--hook key:5 key:5@gprogress5 exec-shell='echo G5' \
--withhold                                         \
--hook key:6 key:6@gprogress5 exec-shell='echo G6' \
--withhold                                         \
--hook key:leftctrl@keyboard$codeForGoogleChrome key:v@keyboard$codeForGoogleChrome send-key=key:leftctrl@$codeForCntrlV \
--withhold key:v@keyboard$codeForGoogleChrome      \
--print key:v format=direct \
`#--hook key:leftctrl@$codeForCntrlV `\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1





