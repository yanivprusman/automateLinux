# bind -x '"\e[1;5B":cat $AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE'
bind -x '"\e[1;5B":awk -v ptr="$AUTOMATE_LINUX_DIR_HISTORY_POINTER" '\''{print (NR==ptr ? "\033[33m"$0"\033[0m" : $0)}'\'' "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"'

# control up
bind -x '"\e[1;5A":". ${AUTOMATE_LINUX_DIR}utilities/execute/execute.sh"'
bind -x '"\e[1;5D":"cd ${AUTOMATE_LINUX_DIR}utilities/execute/"'
# bind -x '"\e[1;5A":". $AUTOMATE_LINUX_BINDINGS_DIR/pd.sh"'
# bind -x '"\e[1;5A": cd ..; READLINE_LINE=""; READLINE_POINT=0'
# bind -x '"\e[1;5A": builtin cd ..; READLINE_LINE=""; READLINE_POINT=0'
# bind -x '"\e[1;5A": builtin cd ..; READLINE_LINE=""; READLINE_POINT=0; printf "\r";'
# bind -x '"\e[1;5A": builtin cd ..; READLINE_LINE=""; READLINE_POINT=0; printf "\e[2K\r";'

# cd_up() {
#     cd .. || return            # go up one directory
#     READLINE_LINE=""           # clear current input line
#     READLINE_POINT=0           # reset cursor position
#     # send newline so bash prints prompt again
#     printf "\n"
# }
# bind -x '"\e[1;5A":cd_up'
