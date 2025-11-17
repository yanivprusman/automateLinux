# echo in commandPrompt: $PWD
# set +x
updateDirHistory

#ps1
tty=$(tty | sed 's/\/dev\/pts\///')
PS1='\[\e]0;'$tty'\w\a\]'"${_yellow}\w${_nc}\$ "
unset tty
