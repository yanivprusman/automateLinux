# control+b     source bindings.sh
bind -x '"\C-b":". ${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh"'
# control+n     new terminal tab
bind -x '"\C-n":"gnome-terminal --tab "'
# control+down  forward a directory
doCdForward() {
    pdd
    echo -ne "\033[2K"
    echo -ne "\033[1A"
    echo -ne "\033[2K"
}
bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null
# control+up    backword a directory
doCdBack() {
    pd
    echo -ne "\033[2K"
    echo -ne "\033[1A"
    echo -ne "\033[2K"
}
bind -s '"\e[1;5A": "doCdBack\n"' >/dev/null
