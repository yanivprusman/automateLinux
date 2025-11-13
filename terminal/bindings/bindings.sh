# bind -x '"\e[1;5B":cat $AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE'
bind -x '"\e[1;5B":awk -v ptr="$AUTOMATE_LINUX_DIR_HISTORY_POINTER" '\''{print (NR==ptr ? "\033[33m"$0"\033[0m" : $0)}'\'' "$AUTOMATE_LINUX_DIR_HISTORY_TTY_FILE"'

# bind -x '"\e[1;7A":clear' not working
bind -x '"\e[1;5A":". $AUTOMATE_LINUX_BINDINGS_DIR/pd.sh"'