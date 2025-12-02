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

printDir(){
    local dirs=()
    local files=()
    local dir f
    local -A excluded_files=()
    local exclude_file=".printDir.sh"
    local mode="dirs"
    local use_color=true
    
    while [ $# -gt 0 ]; do
        case "$1" in
            --help|-h)
                cat <<'EOF'
Usage: printDir [OPTIONS] [DIRECTORIES] [-f FILES]

Print contents of files in directories or individual files with colored output.

Options:
  -d              Process directories (default mode)
  -f              Process files
  --no-color      Disable colored output
  --help, -h      Display this help message

Features:
  - Prints file contents with colored headers and separators
  - Respects .printDir.sh exclude file (lines are comments or filenames to skip)
  - Default behavior: prints all files in current directory

Examples:
  printDir /path/to/dir
  printDir -f file1.txt file2.txt
  printDir /dir1 /dir2 --no-color
  printDir /dir -f file.txt --no-color
EOF
                return 0
                ;;
            --no-color)
                use_color=false
                shift
                ;;
            -d)
                mode="dirs"
                shift
                ;;
            -f)
                mode="files"
                shift
                ;;
            *)
                if [ "$mode" = "dirs" ]; then
                    dirs+=("$1")
                    shift
                else
                    files+=("$1")
                    shift
                fi
                ;;
        esac
    done
    
    if [ ${#dirs[@]} -eq 0 ] && [ ${#files[@]} -eq 0 ]; then
        dirs=(".")
    fi
    
    for dir in "${dirs[@]}"; do
        if [ -d "$dir" ] && [ -f "$dir/$exclude_file" ]; then
            while IFS= read -r line || [ -n "$line" ]; do
                line="${line%%#*}"
                line="${line%"${line##*[![:space:]]}"}"
                line="${line#"${line%%[![:space:]]*}"}"
                [ -z "$line" ] && continue
                excluded_files["$line"]=1
            done < "$dir/$exclude_file"
        fi
    done
    
    if [ ${#dirs[@]} -gt 0 ]; then
        for dir in "${dirs[@]}"; do
            if [ -d "$dir" ]; then
                for f in "${dir%/}/"*; do
                    if [ -f "$f" ]; then
                        local basename_f
                        basename_f=$(basename "$f")
                        [ -z "${excluded_files[$basename_f]}" ] || continue
                        if [ "$use_color" = true ]; then
                            echo -e "${GREEN}$basename_f:${NC}"
                        else
                            echo "$basename_f:"
                        fi
                        cat "$f"
                        if [ "$use_color" = true ]; then
                            echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
                        else
                            echo "${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}"
                        fi
                    fi
                done
            fi
        done
    fi
    
    if [ ${#files[@]} -gt 0 ]; then
        for f in "${files[@]}"; do
            if [ -f "$f" ]; then
                local basename_f
                basename_f=$(basename "$f")
                [ -z "${excluded_files[$basename_f]}" ] || continue
                if [ "$use_color" = true ]; then
                    echo -e "${GREEN}$basename_f:${NC}"
                else
                    echo "$basename_f:"
                fi
                cat "$f"
                if [ "$use_color" = true ]; then
                    echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
                else
                    echo "${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}"
                fi
            fi
        done
    fi
}
export -f printDir

isFile(){
    local file="$1"
    if [ -f "$file" ]; then
        echo true
    else
        echo false
    fi
}
export -f isFile

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
