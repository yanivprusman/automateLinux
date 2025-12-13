# Miscellaneous utilities

password() { python3 ~/generatePassword.py; }

h() {
    history | grep "$@"
}
export -f h

rmr() {
    local dir="${1:-.}"
    rm -r "$dir"/*
}
export -f rmr

lstr(){
    ls --color=always "$@" | tr " " "\n"
}
export -f lstr

alias ls='ls_my'
ls_my() {
    # Check if -l flag is present in arguments
    if [[ "$*" == *"-l"* ]]; then
        # Use ls normally with -l
        command ls --color=always "$@"
    # elif [[ "$*" == *"-l"* ]]; then
    #     # Use ls normally with -l
    #     command ls --color=always "$@"
    else
        # Apply the tr transformation
        command ls --color=always "$@" | tr " " "\n"
    fi
}
export -f ls_my

copyToClipboard(){
    (xclip -selection clipboard)
}
export -f copyToClipboard

setCdc() {
    local newDir="$1"
    aliasFile="${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    if grep -q '^alias cdc=' "$aliasFile"; then
        sed -i "s|^alias cdc=.*|alias cdc='cd \"$newDir\"'|" "$aliasFile"
    fi
    alias cdc="cd \"$newDir\""
}
export -f setCdc

restartGnomeExtensions() {
    gnome-extensions disable clock@ya-niv.com
    gnome-extensions enable clock@ya-niv.com
}
export -f restartGnomeExtensions

gnomeExtensionRestart(){
    gnome-extensions disable $1 && 
    # sleep 1 && 
    gnome-extensions enable $1 && 
    # sleep 2 && 
    echo "Extension reloaded"
}

monitorVSCodeKeyboard(){
    local file=$(d getFile fileName=corsairKeyBoardLogiMouseCode.sh)
    echo monitoring $file
    inotifywait -m -e close_write "$file" | while read _; do
        daemon setKeyboard keyboardName=DefaultKeyboard
        daemon setKeyboard keyboardName=Code
    done
}
export -f monitorVSCodeKeyboard