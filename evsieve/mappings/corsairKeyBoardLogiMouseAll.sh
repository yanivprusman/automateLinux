evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
`#--toggle msc:scan:$codeForAppCodes key:a key:b id=appCodesToggle `\
--output name=corsairKeyBoardLogiMouse 2>&1




