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
    gitPrintChangesFileName="$@"
    git log --pretty=format:'%h %ad %s' --date=short -- name-only "$@" 
}
export -f gitPrintChanges

gitCheckoutLastFile(){
    git checkout "$1" -- "$gitPrintChangesFileName"
}
export -f gitCheckoutLastFile


