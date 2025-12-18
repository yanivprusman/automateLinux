compareCommit(){
    echo -e  "${YELLOW}Commit $1 message:"
    git log --format=%B -n 1 $1 | sed '${/^$/d}'
    echo -e "${YELLOW}Commit $2 message:"
    git log --format=%B -n 1 $2 | sed '${/^$/d}'
    echo -e "${YELLOW}$(git diff --name-only $1 $2 | wc -l) Files changed:${NC}" | sed '${/^$/d}'
    git diff --name-only "$1" "$2" | while IFS= read -r file; do
        echo "$(git log -1 --format="%ct" -- "$file") $file"
    done | sort -rn | cut -d' ' -f2- | while IFS= read -r file; do
        printf "${YELLOW}%s${NC}\n" "$file"
    done
    echo -e "${YELLOW}Diff between $1 and $2:"
    git --no-pager diff -U999 $1 $2
}
export -f compareCommit

gitm(){
    if ! git diff --cached --quiet; then
        git commit -m "$*"
    fi
    git status -sb
}
export -f gitm

gitPrintChanges(){
    git log --pretty=format:'%h %ad %s' --date=short -- name-only "$@" 
}
export -f gitPrintChanges

gitCheckoutLastFile() {
    if [ "$#" -eq 0 ] || [ "$#" -gt 2 ]; then
        echo "Usage: gitCheckoutLastFile <commit> [file]"
        echo "If file is not provided, you will be prompted to select from changed files in the commit."
        return 1
    fi

    local commit="$1"
    local file="$2"

    if ! git rev-parse --verify "$commit"^{commit} >/dev/null 2>&1; then
        echo "Error: Invalid commit hash '$commit'"
        return 1
    fi

    if [ -n "$file" ]; then
        # File is provided, check it out
        echo "Checking out '$file' from commit '$commit'"
        git checkout "$commit" -- "$file"
    else
        # File is not provided, list files in commit and let user choose
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


gitCheckoutLastFile2(){
    git checkout "$1" -- "$gitPrintChangesFileName"
}
export -f gitCheckoutLastFile2

gitDiffFile(){
    if [ "$#" -ne 3 ]; then
        echo "Usage: gitDiffFile <hash1> <hash2> <file_path>"
        return 1
    fi
    git --no-pager diff -U999 "$1" "$2" -- "$3"
}
export -f gitDiffFile





