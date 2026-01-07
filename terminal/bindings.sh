# control+b     source bindings.sh
bind -x '"\C-b":". ${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"'
# control+n     new terminal tab
bind -x '"\C-n":"gnome-terminal --tab "'
# control+down  forward a directory
# doCdForward() {
#     local dirB4=$PWD
#     local result=$(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
#     $($result)
#     if $dirB4 != $PWD; then
#         echo -ne "\033[2K\033[1A\033[2K"
#     fi
#     history -d -1
# }
# bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null

# control+down  forward a directory
doCdForward() { 
    restoreOutput
    local dirB4=$PWD
    local result=$(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    if echo "$result" | grep -q "^cd "; then
        echo -ne "\033[2K\033[1A\033[2K"
        $result
    else
        echo -e "\033[2K\033[1A\033[2K"
        $result
        echo -ne "\033[2A"
    fi
    # redirectSession
    history -d -1
}
bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null

# control+up    backward a directory
doCdBack() {
    restoreOutput
    local dirB4=$PWD
    local result=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    if echo "$result" | grep -q "^cd "; then
        echo -ne "\033[2K\033[1A\033[2K"
        $result
    else
        echo -e "\033[2K\033[1A\033[2K"
        $result
        echo -ne "\033[2A"
    fi
    # redirectSession
    history -d -1
}
bind -s '"\e[1;5A": "doCdBack\n"' >/dev/null
# doCdForward() {
#     echo $(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
# }
# bind -s '"\e[1;5B": "doCdForward\n"' >/dev/null
# # control+up    backward a directory
# doCdBack() {
#     echo $(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
# }
# bind -s '"\e[1;5A": "doCdBack\n"' >/dev/null
