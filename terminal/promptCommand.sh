#pd and pdd commands
updateDirHistory
# local pointedToDir
# if [[ ! resetDirHistoryToBeginningStateIfError ]]; then
#     pointedToDir=$(getDirFromHistory)
#     if [[ -z "$pointedToDir" ]]; then
#         resetDirHistoryToBeginningState
#     fi
# if [[ "$PWD" != "$pointedToDir" ]]; then #in pd case this evaluates to false since pwd will return the same directory that's at the current history pointer
#     insertDirAfterIndex $PWD $AUTOMATE_LINUX_DIR_HISTORY_POINTER
#     AUTOMATE_LINUX_DIR_HISTORY_POINTER=$((AUTOMATE_LINUX_DIR_HISTORY_POINTER + 1))
#     totalLines=$(wc -l < "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE")
#     if [[ "$AUTOMATE_LINUX_DIR_HISTORY_POINTER" -gt "$totalLines" ]]; then
#         AUTOMATE_LINUX_DIR_HISTORY_POINTER=$totalLines
#     fi
#     setDirHistoryPointer "$AUTOMATE_LINUX_ESCAPED_TTY" "$AUTOMATE_LINUX_DIR_HISTORY_POINTER"
# else
#     touch "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"
# fi 
# cd /home/yaniv/coding/automateLinux
# cd ..
#ps1
tty=$(tty | sed 's/\/dev\/pts\///')
PS1='\[\e]0;'$tty'\w\a\]'"${_yellow}\w${_NC}\$ "
unset tty
