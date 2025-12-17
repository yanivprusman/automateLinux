# daemon updateDirHistory - suppress output since we only care about the side effects
daemon send updateDirHistory --tty $AUTOMATE_LINUX_TTY_NUMBER --pwd "${PWD}/" > /dev/null 
# 2>&1
# PS1='\[\e]0;'$AUTOMATE_LINUX_TTY_NUMBER'\w\a\]'"${_yellow}\w${_nc}\$ "
