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
    if [[ "$output" == *echo* ]]; then
        # Clean up signal state if it was set from a previous navigation
        if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
            echo -ne "\r\033[2K\033[1A\033[2K"
            AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
        fi
        echo -ne "\r\033[2K"
        eval "$output"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        # If the last navigation was a signal, move up and clear
        if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
            echo -ne "\r\033[2K\033[1A\033[2K"
            AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
        fi

        # Clear current line where binding might have leaked chars
        echo -ne "\r\033[2K"
        eval "$output"
        
        # Force prompt redraw via daemon signal command
        # This works around bind -x blocking signals
        local redraw_cmd
        redraw_cmd=$(daemon send shellSignal --signal WINCH)
        eval "$redraw_cmd"
    fi
    history -d -1
}
bind -x '"\e[1;5B": "doCdForward"'

# control+up    backward a directory
doCdBack() {
    local output
    output=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")

    if [[ "$output" == *echo* ]]; then
        echo -ne "\r\033[2K"
        eval "$output"
        AUTOMATE_LINUX_LAST_NAV_SIGNAL="1"
    else
        # If the last navigation was a signal, move up and clear
        if [[ -n "$AUTOMATE_LINUX_LAST_NAV_SIGNAL" ]]; then
            echo -ne "\r\033[2K\033[1A\033[2K"
            AUTOMATE_LINUX_LAST_NAV_SIGNAL=""
        fi

        echo -ne "\r\033[2K"
        eval "$output"
        
        # Force prompt redraw via daemon signal command
        local redraw_cmd
        redraw_cmd=$(daemon send shellSignal --signal WINCH)
        eval "$redraw_cmd"
    fi
    history -d -1
}
bind -x '"\e[1;5A": "doCdBack"'
