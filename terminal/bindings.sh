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
    
    # Clean up signal state
    if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
        echo -ne "\r\033[2K\033[1A\033[2K"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
    fi

    if [[ "$output" == *echo* ]]; then
        echo -ne "\r\033[2K"
        eval "$output"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        # Clear the line where bind -x might have printed something
        echo -ne "\r\033[2K"
        eval "$output"
        # Force prompt redraw by touching readline state
        READLINE_LINE=" "
        READLINE_POINT=1
        READLINE_LINE=""
        READLINE_POINT=0
    fi
    history -d -1
}
bind -x '"\e[1;5B": "doCdForward"'

# control+up    backward a directory
doCdBack() {
    local output
    output=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")

    # Clean up signal state
    if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
        echo -ne "\r\033[2K\033[1A\033[2K"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
    fi

    if [[ "$output" == *echo* ]]; then
        echo -ne "\r\033[2K"
        eval "$output"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        echo -ne "\r\033[2K"
        eval "$output"
        # Force prompt redraw by touching readline state
        READLINE_LINE=" "
        READLINE_POINT=1
        READLINE_LINE=""
        READLINE_POINT=0
    fi
    history -d -1
}
bind -x '"\e[1;5A": "doCdBack"'
