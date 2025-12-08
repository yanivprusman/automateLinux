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
    command ls --color=always "$@" | tr " " "\n"
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