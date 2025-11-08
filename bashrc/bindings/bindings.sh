bind -x '"\e[1;5B":cat $AUTOMATE_LINUX_DIR_HISTORY_FILE_TTY'
# bind -x '"\e[1;7A":clear' not working
bind -x '"\e[1;5A":". $AUTOMATE_LINUX_BINDINGS_DIR/pd.sh"'