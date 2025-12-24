evsieve --input $keyboardPath $mousePath grab domain=input \
--map btn:forward key:enter \
--toggle msc:scan:$codeForAppCodes @pressed1 @pressed2 id=appCodesToggle \
--toggle key:a @pressed1 @pressed2 id=aToggle \
--output create-link=/dev/input/by-id/corsairKeyBoardLogiMouse 2>&1




