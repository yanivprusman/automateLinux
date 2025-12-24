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
    local newDir="${1:-$PWD}"
    aliasFile="${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    if grep -q '^alias cdc=' "$aliasFile"; then
        sed -i "s|^alias cdc=.*|alias cdc='cd \"$newDir\"'|" "$aliasFile"
    fi
    alias cdc="cd \"$newDir\""
}
export -f setCdc

restartGnomeExtensions() {
    # gnome-extensions disable clock@ya-niv.com
    # gnome-extensions enable clock@ya-niv.com
    # killall -HUP gnome-shell #test it at least once
    gnome-shell -r & #test it at least once
}
export -f restartGnomeExtensions

tmuxReloadConfig() {
    tmux source-file ~/.tmux.conf
}
export -f tmuxReloadConfig

bd(){
    local caller_dir="$PWD"
    cd "$AUTOMATE_LINUX_DAEMON_DIR"
    bs
    cd "$caller_dir" >/dev/null
}
export -f bd

showSymlinkHistory() {
    local SYMLINK=$1
    if [ -z "$SYMLINK" ]; then
        echo "Usage: showSymlinkHistory path/to/symlink"
        return 1
    fi

    git log -p --follow --abbrev-commit --date=format:'%d/%m/%y' \
        --pretty=format:"commit %h %ad" -- "$SYMLINK" | \
    awk '
/^commit/ { 
    if(old || new) { 
        type=(old && new) ? "Updated" : (old ? "Deleted" : "Created")
        printf "%s | %s | %s | %s -> %s\n", commit, date, type, old?old:"--", new?new:"--"
    }
    commit=$2; date=$3; old=""; new=""; next
}
/^-([^-\n])/ { old=substr($0,2) }
/^\+([^+\n])/ { new=substr($0,2) }
END {
    if(old || new) {
        type=(old && new) ? "Updated" : (old ? "Deleted" : "Created")
        printf "%s | %s | %s | %s -> %s\n", commit, date, type, old?old:"--", new?new:"--"
    }
}'
}
export -f showSymlinkHistory

makeVscePackage(){
    npm run compile
    vsce package
    code --install-extension *.vsix
}

geminiFree(){
    gemini --model gemini-2.5-flash "$@"
}
export -f makeVscePackage

j (){
    journalctl /usr/bin/gnome-shell -n 100 --no-pager
}
export -f j

updateGeminiVersion(){
    sudo npm install -g @google/gemini-cli
    gemini -v
}
export -f updateGeminiVersion

evsievep() {
    local path
    if [[ " $* " == *" -k "* ]]; then
        path=$(d send getKeyboardPath)
    else
        path="/dev/input/event*"
    fi
    sudo evsieve --input $path --print format=direct
}
export -f evsievep