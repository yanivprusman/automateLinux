# bind -x '"\e[1;5B":cat $AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE'
bind -x '"\e[1;5B":awk -v ptr="$AUTOMATE_LINUX_DIR_HISTORY_POINTER" '\''{print (NR==ptr ? "\033[33m"$0"\033[0m" : $0)}'\'' "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"'
bind '"\e[1;5D": backward-word'
# control n
bind -x '"\C-n":"gnome-terminal --tab "'
# control b
bind -x '"\C-b":". ${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh"'

# control up
# bind -x '"\e[1;5A":". ${AUTOMATE_LINUX_DIR}utilities/execute/execute.sh"'
# bind -x '"\e[1;5A": cd ..'
# bind '"\e[1;5A": redraw-current-line'
# control left
# bind -x '"\e[1;5D":"cd ${AUTOMATE_LINUX_DIR}utilities/execute/"'
# bind -r "\e[1;5D"
# bind -x '"\e[1;5D": backward-word'
# my_cd_and_redraw() {
#     cd ..
#     bind '"\e[1;5A": redraw-current-line'

# }
# bind -x '"\e[1;5A": my_cd_and_redraw'
# export -f my_cd_and_redraw
# bind '"\e[1;5A": my_cd_and_redraw'
bind -s '"\e[1;5A": "cd ..\n"'
