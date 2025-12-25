evsieve --input $keyboardPath $mousePath grab domain=input \
--map key @gProgress                                            `#part of detecting G keys`\
--copy key @keyboard                                              `#part of detecting window`\
`#`\
--map btn:forward key:enter                                                `#mouse to enter`\
--hook key:leftctrl key:1 exec-shell='sudo -u yaniv DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus DISPLAY=:0 notify-send "hi" "2"'`#sanity check` \
--hook key:leftctrl key:1 exec-shell='echo >/home/yaniv/coding/automateLinux/data/evsieveOutput.log'`#clear log` \
`#--print key format=direct `\
                                                               `#detecting window continued`\
--toggle @gProgress @gProgress1 @gProgress2 @gProgress3 @gProgress4 @gProgress5 id=GToggle  \
--hook msc:scan:$codeForAppCodes toggle=appCodesToggle         \
--map msc:scan:$codeForAppCodes @codeForAppCodesCount                                       \
--toggle @codeForAppCodesCount @pressed1 @pressed2 @pressed3 id=appCodesToggle              \
--hook @pressed3 toggle=appCodesToggle:2                                                    \
--hook @pressed3 msc:scan:$codeForCode toggle=keyboardToggle:1                              \
--hook @pressed3 msc:scan:$codeForGnomeTerminal toggle=keyboardToggle:2                     \
--hook @pressed3 msc:scan:$codeForGoogleChrome toggle=keyboardToggle:3                            \
`#      detecting G keys continued                `\
`#     1       2      3       4     5             `\
`#  ctrl10  shift10 ctrl10 shift10 N10            `\
--hook key:leftctrl@gProgress1 toggle=GToggle:2    \
--hook @gProgress2 toggle=GToggle:1                \
--hook key:leftshift@gProgress2 toggle=GToggle:3   \
--hook @gProgress3 toggle=GToggle:1                \
--hook key:leftctrl@gProgress3 toggle=GToggle:4    \
--hook @gProgress4 toggle=GToggle:1                \
--hook key:leftshift@gProgress4 toggle=GToggle:5   \
--hook @gProgress5 toggle=GToggle:1                \
--print @nothing                                   \
--hook key:1@gProgress5 key:1@keyboard exec-shell='echo G1' breaks-on=key::1 sequential --withhold \
--hook key:2@gProgress5 key:2@keyboard exec-shell='echo G2' breaks-on=key::1 sequential --withhold \
--hook key:3@gProgress5 key:3@keyboard exec-shell='echo G3' breaks-on=key::1 sequential --withhold \
--hook key:4@gProgress5 key:4@keyboard exec-shell='echo G4' breaks-on=key::1 sequential --withhold \
--hook key:5@gProgress5 key:5@keyboard exec-shell='echo G5' breaks-on=key::1 sequential --withhold \
--hook key:6@gProgress5 key:6@keyboard exec-shell='echo G6' breaks-on=key::1 sequential --withhold \
--block @gProgress @gProgress1 @gProgress2 @gProgress3 @gProgress4 @gProgress5 \
--toggle @keyboard @keyboard$codeForCode @keyboard$codeForGnomeTerminal @keyboard$codeForGoogleChrome id=keyboardToggle   \
--hook key:leftctrl@keyboard$codeForGoogleChrome key:v@keyboard$codeForGoogleChrome send-key=key:leftctrl@$codeForCntrlV   \
--withhold key:v@keyboard$codeForGoogleChrome                                                                              \
--map key:leftctrl:1@$codeForCntrlV key:leftctrl:0@$codeForCntrlV \
`#--print key:a@nothing key:v@nothing key:leftctrl format=direct `\
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1


