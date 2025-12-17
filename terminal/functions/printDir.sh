#!/bin/bash
print2(){
    echo "This is print2"
}
print(){
    local -A excluded_files=()
    local -a dirs=()
    local -a files=() dir f exclude_file=".printDir.sh" mode="dirs" use_color=true copy_to_clipboard=false
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
  --copy, -c      Copy output to clipboard (implies --no-color)
  --help, -h      Display this help message

Features:
  - Prints file contents with colored headers and separators
  - Respects .printDir.sh exclude file (lines are comments or filenames to skip)
  - Default behavior: prints all files in current directory

Examples:
  printDir /path/to/dir
  printDir -f file1.txt file2.txt
  printDir /dir1 /dir2 --no-color
  printDir /dir -f file.txt -copy
EOF
                return 0
                ;;
            --no-color)
                use_color=false
                shift
                ;;
            --copy|-c)
                copy_to_clipboard=true
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
    
    local output
    {
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
                    [ -n "$(tail -c1 "$f")" ] && echo
                    if [ "$use_color" = true ]; then
                        echo -e "${YELLOW}${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}${NC}"
                    else
                        echo "${AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR}"
                    fi
                fi
            done
        fi
    } | if [ "$copy_to_clipboard" = true ]; then
        xclip -selection clipboard
    else
        cat
    fi
}
export -f print
