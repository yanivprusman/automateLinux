# control+b     source bindings.sh
bind -x '"\C-b":". ${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"'
# control+n     new terminal tab
bind -x '"\C-n":"gnome-terminal --tab "'
# control+down  forward a directory
# doCdForward() {
#     $(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
#     echo -ne "\033[2K\033[1A\033[2K"
#     history -d -1
# }
# bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null
# # control+up    backward a directory
# doCdBack() {
#     $(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
#     echo -ne "\033[2K\033[1A\033[2K"
#     history -d -1
# }
# bind -s '"\e[1;5A": "doCdBack\n"' >/dev/null
doCdForward() {
    echo $(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
}
bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null
# control+up    backward a directory
doCdBack() {
    echo $(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
}
bind -s '"\e[1;5A": "doCdBack\n"' >/dev/null
