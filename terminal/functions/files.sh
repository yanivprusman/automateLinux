# File and directory utilities

catDir() {
    local dir="$1"
    if [ -z "$dir" ]; then dir="."; fi
    if [ -d "$dir" ]; then
        for file in "$dir"*; do
            if [ -f "$file" ]; then
                echo -e "${green}----- Contents of $file:${NC}"
                cat "$file"
            fi
        done
    else
        echo "$dir is not a directory."
    fi
}
export -f catDir

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
