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