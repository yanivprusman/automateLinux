# File and directory utilities

heredoc() {
    local file="$1"
    if [ -f "$file" ]; then
        local outputFile=$(basename "${1%.*}").hereDoc
        echo "bash <<'EOF'" > "$outputFile"
        cat "$file" >> "$outputFile"
        echo "EOF" >> "$outputFile"
    fi
}
export -f heredoc

isFile(){
    local file="$1"
    if [ -f "$file" ]; then
        echo true
        return 0
    else
        echo false
        return 1
    fi
}
export -f isFile

isDir(){
    local dir="$1"
    if [ -d "$dir" ]; then
        echo true
        return 0
    else
        echo false
        return 1
    fi
}
export -f isDir

isPath(){
    local p="$1"
    if [ -f "$p" ]; then
        echo file
        return 0
    elif [ -d "$p" ]; then
        echo dir
        return 0
    else
        echo none
        return 1
    fi
}
export -f isPath


lsd(){
    ls -d */
}
export -f lsd

touchDirectories() {
    local dir
    local automateLinuxVariable
    compgen -v | grep '^AUTOMATE_LINUX_' | grep '_DIR$' | while read -r automateLinuxVariable; do
        dir="${!automateLinuxVariable}"
        if [ ! -d "$dir" ]; then
            mkdir -p "$dir"
        fi
    done
}
export -f touchDirectories

lastChangedFiles() {
    $(theRealPath "${AUTOMATE_LINUX_DIR}/utilities/lastChanged/lastChanged" "$@")
}
export -f lastChangedFiles 

lastChanged() {
    lastChangedFiles | 
    sed "s|$PWD/||" |
    while IFS= read -r f; do
        ls -l --color=always --time-style="+%d/%m/%Y %H:%M" -- "$f"
    done | 
    awk '{print $8 "\t" $6, $7}' | 
    column -t -s $'\t'
}
export -f lastChanged

cleanBetween() {
    $(theRealPath "${AUTOMATE_LINUX_DIR}/utilities/cleanBetween/cleanBetween" "$@")
}