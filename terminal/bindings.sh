# control+b     source bindings.sh
bind -x '"\C-b":". ${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"'
# control+n     new terminal tab
bind -x '"\C-n":"gnome-terminal --tab "'
# State variable to track if the last navigation showed a boundary signal
# so we can overwrite it instead of piling up.
# This variable is per-shell session.
AUTOMATE_LINUX_LAST_NAV_SIGNAL=""

# control+down  forward a directory
doCdForward() {
    local output
    output=$(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    
    # If the last navigation was a signal, clear it before printing anything new
    if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
        echo -ne "\033[2K\033[1A\033[2K"
    fi

    eval "$output"

    if [[ "$output" == *echo* ]]; then
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
        # Clear the "phantom" line created by bind -x if we did a real 'cd'
        echo -ne "\033[2K\033[1A\033[2K"
    fi
    history -d -1
}
bind -x '"\e[1;5B": "doCdForward"'

# control+up    backward a directory
doCdBack() {
    local output
    output=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")

    # If the last navigation was a signal, clear it before printing anything new
    if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
        echo -ne "\033[2K\033[1A\033[2K"
    fi

    eval "$output"

    if [[ "$output" == *echo* ]]; then
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
        echo -ne "\033[2K\033[1A\033[2K"
    fi
    history -d -1
}
bind -x '"\e[1;5A": "doCdBack"'
