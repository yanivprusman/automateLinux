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

makeVscePackageforVSCode(){
    npm run compile
    vsce package --allow-missing-repository --skip-license
    code --install-extension *.vsix --force
}
export -f makeVscePackageforVSCode

geminiFree(){
    gemini --model gemini-2.5-flash "$@"
}

j (){
    journalctl /usr/bin/gnome-shell -n 100 --no-pager
}
export -f j

updateGeminiVersion(){
    sudo npm install -g @google/gemini-cli
    gemini -v
}
export -f updateGeminiVersion

toHex() {
  printf "0x%X\n" "$1"
}
export -f toHex

toDecimal() {
  printf "%d\n" "$1"
}
export -f toDecimal

listMyFunctions() {
    grep -rhE "^[a-zA-Z0-9_-]+[[:space:]]*\(\)[[:space:]]*\{" "$AUTOMATE_LINUX_TERMINAL_DIR"/*.sh "$AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR"/*.sh 2>/dev/null | \
        sed -E "s/^([a-zA-Z0-9_-]+).*/\1/" | sort -u
}
export -f listMyFunctions

emergencyRestore() {
    $AUTOMATE_LINUX_DIR/utilities/emergencyRestore.sh
}
export -f emergencyRestore

tailCombinedLog() {
    local erase=false
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -e|--erase)
                erase=true
                shift
                ;;
            *)
                echo "Unknown option: $1" >&2
                return 1
                ;;
        esac
    done
    local logfile="${AUTOMATE_LINUX_DIR}/data/combined.log"
    if [[ "$erase" == true ]]; then
        : > "$logfile" || return 1
    fi
    tail -f "$logfile" | awk '{ print strftime("[%Y-%m-%d %H:%M:%S]"), $0 }'
}
export -f tailCombinedLog

clc() {
    "$@" 2>&1 | tee /dev/tty | tail -n 1 | xclip -selection clipboard
}
export -f clc

moveAllToSubdir(){
    if [ -z "$1" ]; then
        echo "Usage: moveAllToSubdir <subdirectory>"
        return 1
    fi
    shopt -s extglob
    local subdir=${1%/}  # Remove trailing slash if present
    mv !("$subdir") "$subdir"/
}
# export -f moveAllToSubdir

openFunction(){
    # code # use type $1 to see the function and find the script it is in and then open that script in code
    local func_name=$1
    if [ -z "$func_name" ]; then
        echo "Usage: openFunction <function_name>"
        return 1
    fi
    local func_file
    func_file=$(grep -rlE "^${func_name}[[:space:]]*\(\)[[:space:]]*\{" "$AUTOMATE_LINUX_TERMINAL_DIR"/*.sh "$AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR"/*.sh 2>/dev/null | head -n 1)
    if [ -z "$func_file" ]; then
        echo "Function '$func_name' not found."
        return 1
    fi
    code "$func_file"
}

openSpecialChrome(){
    exec -a "not-chrome" google-chrome
}

openGnomeSession(){
    # dbus-run-session -- bash
    dbus-run-session gnome-shell --display-server --wayland
}

setDarkMode(){
    gsettings set org.gnome.desktop.interface color-scheme 'prefer-dark'
}

