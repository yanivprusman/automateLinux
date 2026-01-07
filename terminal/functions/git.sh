gitCompareCommit(){
    local commit1=""
    local commit2=""
    local specific_file=""
    while (( "$#" )); do
        case "$1" in
            -f)
                if [ -n "$2" ] && [[ ! "$2" =~ ^- ]]; then
                    specific_file="$2"
                    shift 2
                else
                    echo "Error: -f requires a file path."
                    echo "Usage: gitCompareCommit <commit1> <commit2> [-f <file_path>]"
                    return 1
                fi
                ;;
            *)
                if [ -z "$commit1" ]; then
                    commit1="$1"
                elif [ -z "$commit2" ]; then
                    commit2="$1"
                else
                    echo "Error: Too many arguments."
                    echo "Usage: gitCompareCommit <commit1> <commit2> [-f <file_path>]"
                    return 1
                fi
                shift
                ;;
        esac
    done
    if [ -z "$commit1" ] || [ -z "$commit2" ]; then
        echo "Usage: gitCompareCommit <commit1> <commit2> [-f <file_path>]"
        return 1
    fi
    local is_commit1_valid=false
    if git rev-parse --verify "$commit1"^{commit} >/dev/null 2>&1; then
        is_commit1_valid=true
        echo -e "${YELLOW}Commit $commit1 message:${NC}"
        git log --format=%B -n 1 "$commit1" | sed '${/^$/d}'
    else
        echo -e "${YELLOW}Commit $commit1 message:${NC} Does not exist."
    fi
    local is_commit2_valid=false
    if git rev-parse --verify "$commit2"^{commit} >/dev/null 2>&1; then
        is_commit2_valid=true
        echo -e "${YELLOW}Commit $commit2 message:${NC}"
        git log --format=%B -n 1 "$commit2" | sed '${/^$/d}'
    else
        echo -e "${YELLOW}Commit $commit2 message:${NC} Does not exist."
    fi

    if ! $is_commit1_valid || ! $is_commit2_valid; then
        return 1
    fi
    if [ -z "$specific_file" ]; then
        local files_changed=$(git diff --name-only "$commit1" "$commit2")
        local num_files=$(echo "$files_changed" | wc -l)
        echo -e "${YELLOW}$num_files Files changed:${NC}"
        if [ -n "$files_changed" ]; then
            echo "$files_changed" | while IFS= read -r file; do
                printf "${YELLOW}%s${NC}\n" "$file"
            done
        fi
    fi
    echo -e "${YELLOW}Diff between $commit1 and $commit2: ${specific_file:+($specific_file)}${NC}"
    if [ -n "$specific_file" ]; then
        git --no-pager diff -U999 "$commit1" "$commit2" -- "$specific_file"
    else
        git --no-pager diff -U999 "$commit1" "$commit2"
    fi
}
export -f gitCompareCommit

gitm(){
    if ! git diff --cached --quiet; then
        git commit -m "$*"
    fi
    git status -sb
}
export -f gitm

gitPrintChanges(){
    gitPrintChangesFileName="$@"
    git --no-pager log --pretty=format:'%h %ad %s' --date=short -- name-only "$@" 
    echo
}
export -f gitPrintChanges

gitCheckoutLastFile() {
    local commit
    local file
    if [ "$#" -eq 0 ]; then
        echo "Usage: gitCheckoutLastFile <commit> [file]"
        echo "If file is not provided, it will default to the last file used with gitPrintChanges, or prompt for selection."
        if [ -n "$gitPrintChangesFileName" ]; then
            echo "Current default file to use = $gitPrintChangesFileName"
        else
            echo "Currently no default file was set from previous gitPrintChanges call."
        fi
        return 1
    elif [ "$#" -eq 1 ]; then
        commit="$1"
        if [ -n "$gitPrintChangesFileName" ]; then
            file="$gitPrintChangesFileName"
            # echo "Defaulting to file: '$file' from previous gitPrintChanges call."
        else
            file="" # No default, proceed to interactive selection
        fi
    elif [ "$#" -eq 2 ]; then
        commit="$1"
        file="$2"
    else
        echo "Usage: gitCheckoutLastFile <commit> [file]"
        echo "If file is not provided, it will default to the last file used with gitPrintChanges, or prompt for selection."
        return 1
    fi
    if ! git rev-parse --verify "$commit"^{commit} >/dev/null 2>&1; then
        echo "Error: Invalid commit hash '$commit'"
        return 1
    fi
    if [ -n "$file" ]; then
        # File is provided or defaulted, check it out
        echo "Checking out '$file' from commit '$commit'"
        git checkout "$commit" -- "$file"
    else
        # File is not provided and no default, list files in commit and let user choose
        local files_str
        files_str=$(git diff-tree --no-commit-id --name-only -r "$commit")
        if [ -z "$files_str" ]; then
            echo "No files changed in commit $commit"
            return 0
        fi
        echo "Files changed in commit $commit:"
        local files_arr
        mapfile -t files_arr <<< "$files_str"
        PS3="Select a file to checkout: "
        select f in "${files_arr[@]}"; do
            if [ -n "$f" ]; then
                echo "Checking out '$f' from commit '$commit'"
                git checkout "$commit" -- "$f"
                break
            else
                echo "Invalid selection. Try again."
            fi
        done
    fi
}
export -f gitCheckoutLastFile

gitDiffFile(){
    if [ "$#" -ne 3 ]; then
        echo "Usage: gitDiffFile <hash1> <hash2> <file_path>"
        return 1
    fi
    # echo -e  "${YELLOW}Commit $1 message:"
    # git log --format=%B -n 1 $1 | sed '${/^$/d}'
    echo -en  "${YELLOW}"
    git --no-pager log --pretty=format:'%h %ad %s' --date=format:'%d-%m-%Y %H:%M:%S' -n 1 "$1"
    echo
    git --no-pager log --pretty=format:'%h %ad %s' --date=format:'%d-%m-%Y %H:%M:%S' -n 1 "$2"
    echo
    git --no-pager diff -U999 "$1" "$2" -- "$3"
}
export -f gitDiffFile

gitShowFileAtCommit(){
    if [ "$#" -ne 2 ]; then
        echo "Usage: gitShowFileAtCommit <commit_hash> <file_path>"
        return 1
    fi
    git --no-pager show "$1":"$2"
}
export -f gitShowFileAtCommit

gitAnnotateChanges() {
  local fromCommit="$1"
  local toCommit="$2"
  local file="$3"

  git --no-pager diff -U999 "$fromCommit" "$toCommit" -- "$file" | \
awk '
/^@@/ { next }      # skip hunk headers
/^---/ { next }      # skip file headers
/^\+\+\+/ { next }   # skip file headers
/^-/{ sub(/^-/, ""); print $0 " //remove"; next }
/^\+/{ sub(/^\+/, ""); print "//" $0 " //add"; next }
/^ /{ sub(/^ /, ""); print }'
}

gitw(){
    gitm "wip"
}

gitl(){
    git l
}

gita(){
    git a
}

gits(){
    git s
}