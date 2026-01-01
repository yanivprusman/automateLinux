# control+b     source bindings.sh
bind -x '"\C-b":". ${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"'
# control+n     new terminal tab
bind -x '"\C-n":"gnome-terminal --tab "'
# control+down  forward a directory
doCdForward() {
    local output
    output=$(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    eval "$output"
    if [[ "$output" != *echo* ]]; then
        echo -ne "\033[2K\033[1A\033[2K"
    fi
    history -d -1
}
bind -x '"\e[1;5B": "doCdForward"'
# control+up    backward a directory
doCdBack() {
    local output
    output=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    eval "$output"
    if [[ "$output" != *echo* ]]; then
        echo -ne "\033[2K\033[1A\033[2K"
    fi
    history -d -1
}
bind -x '"\e[1;5A": "doCdBack"'
