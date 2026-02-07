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
    # Fetch latest from remote silently
    local fetch_err
    fetch_err=$(git fetch origin main 2>&1) || echo "$fetch_err" >&2

    # Get commit counts: left=behind (remote), right=ahead (local)
    local counts
    counts=$(git rev-list --left-right --count origin/main...HEAD 2>/dev/null)

    if [ -n "$counts" ]; then
        local behind ahead
        behind=$(echo "$counts" | cut -f1)
        ahead=$(echo "$counts" | cut -f2)

        if [ "$behind" -gt 0 ]; then
            echo -e "${YELLOW}Warning: origin/main has $behind commit(s) not pulled yet.${NC}"
            read -p "Continue anyway? [y/N] " confirm
            if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
                echo "Aborted."
                return 1
            fi
        fi
    fi

    git a
}

gits(){
    git s
}

gitp(){
    git push
}

gitP(){
    # Check if we're in a git repo
    if ! git rev-parse --is-inside-work-tree &>/dev/null; then
        echo "Not a git repository"
        return 1
    fi

    local branch
    branch=$(git branch --show-current 2>/dev/null)
    local stashed=false

    # Check for uncommitted changes
    if [[ -n $(git status --porcelain) ]]; then
        echo -e "${YELLOW}Warning: You have uncommitted changes${NC}"
        git status -sb
        echo
        read -p "Stash changes before pulling? [y/N] " confirm
        if [[ "$confirm" =~ ^[Yy]$ ]]; then
            git stash || return 1
            stashed=true
        else
            echo -e "${YELLOW}Proceeding without stash (may cause merge conflicts)${NC}"
        fi
    fi

    # Fetch and show ahead/behind before pulling
    git fetch origin "$branch" &>/dev/null

    local counts behind ahead
    counts=$(git rev-list --left-right --count "origin/$branch...HEAD" 2>/dev/null)
    if [ -n "$counts" ]; then
        behind=$(echo "$counts" | cut -f1)
        ahead=$(echo "$counts" | cut -f2)
    fi

    # Pull if behind
    if [ "${behind:-0}" -gt 0 ]; then
        echo -e "Pulling ${YELLOW}$behind${NC} commit(s)..."
        local before_head
        before_head=$(git rev-parse HEAD)

        if git pull; then
            local after_head
            after_head=$(git rev-parse HEAD)
            echo -e "${GREEN}✓ Pulled${NC} $(git rev-parse --short "$before_head")..$(git rev-parse --short "$after_head")"
        else
            echo -e "${RED}✗ Pull failed${NC}"
            if [ -n "$(git ls-files -u)" ]; then
                echo -e "${YELLOW}Merge conflicts detected. Resolve them and commit.${NC}"
            fi
            return 1
        fi
    else
        echo "Already up to date with remote."
    fi

    # Pop stash if we stashed
    if [ "$stashed" = true ]; then
        echo "Restoring stashed changes..."
        git stash pop || return 1
    fi

    # Commit if there are changes
    if [[ -n $(git status --porcelain) ]]; then
        echo
        git status -sb
        echo
        read -p "Commit message (empty to skip): " msg
        if [ -n "$msg" ]; then
            git add -A
            if git commit -m "$msg"; then
                echo -e "${GREEN}✓ Committed${NC}"
            else
                echo -e "${RED}✗ Commit failed${NC}"
                return 1
            fi
        else
            echo "Skipping commit."
            return 0
        fi
    fi

    # Push if ahead
    counts=$(git rev-list --left-right --count "origin/$branch...HEAD" 2>/dev/null)
    if [ -n "$counts" ]; then
        ahead=$(echo "$counts" | cut -f2)
    fi

    if [ "${ahead:-0}" -gt 0 ]; then
        echo -e "Pushing ${YELLOW}$ahead${NC} commit(s)..."
        if git push; then
            echo -e "${GREEN}✓ Pushed${NC}"
        else
            echo -e "${RED}✗ Push failed${NC}"
            return 1
        fi
    else
        echo "Nothing to push."
    fi

    return 0
}
export -f gitP

gitd(){
    git diff
}

gitSync(){
    if [[ -n $(git status --porcelain) ]]; then
        git stash && git pull && git stash pop
    else
        git pull
    fi
}

gitMakeMain() {
  if [ -z "$1" ]; then
    echo "Usage: gitMakeMain <commit-hash>"
    return 1
  fi
  echo "This will run the following steps:"
  echo "1) git switch main"
  echo "2) git reset --hard $1"
  echo "3) git push --force origin main"
  echo
  read -p "Proceed with step 1 (git switch main)? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || return 1
  git switch main || return 1
  read -p "Proceed with step 2 (git reset --hard $1)? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || return 1
  git reset --hard "$1" || return 1
  read -p "Proceed with step 3 (git push --force origin main)? [y/N] " confirm
  [[ "$confirm" =~ ^[Yy]$ ]] || return 1
  git push --force origin main
}

gitHours() {
    git log --since="$1 hours ago" --pretty=format:"%h %ad %s" --date=format:"%d/%m %H:%M"
    echo
}

gitCount(){
    git rev-list --count HEAD
}

gitAheadBehind(){
    # Suppress fetch noise on success, but show output on failure (git prints everything to stderr)
    local fetch_err
    fetch_err=$(git fetch origin main 2>&1) || echo "$fetch_err" >&2
    local counts
    counts=$(git rev-list --left-right --count origin/main...HEAD 2>/dev/null)
    if [ -z "$counts" ]; then
        echo "Error: Could not get commit counts (not a git repo or no origin/main?)"
        return 1
    fi
    local behind ahead
    behind=$(echo "$counts" | cut -f1)
    ahead=$(echo "$counts" | cut -f2)
    echo "Behind: $behind  Ahead: $ahead"
}

addSafeGitDir() {
    sudo git config --system --add safe.directory "$(pwd)"
}

gitsAll() {
    local start_dir="${1:-.}"
    local found_any=false
    local clean_output="" dirty_output=""
    local total_count=0 clean_count=0 dirty_count=0
    local CYAN='\e[0;36m'
    local sep="$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR"

    _gitsAll_format_repo() {
        local repo_dir="$1"
        local repo_name="$2"

        # Get status (no color so we can parse cleanly)
        local status_output
        status_output=$(git -C "$repo_dir" -c color.status=never status -sb 2>/dev/null)

        # Parse the branch line for a human-readable summary
        local branch_line
        branch_line=$(echo "$status_output" | head -1)
        local branch_info=""
        if [[ "$branch_line" =~ ^##\ (.+)\.\.\.(.+) ]]; then
            local local_branch="${BASH_REMATCH[1]}"
            branch_info="${CYAN}${local_branch}${NC}"
            if [[ "$branch_line" =~ \[ahead\ ([0-9]+),\ behind\ ([0-9]+)\] ]]; then
                branch_info="${CYAN}${local_branch}${NC} ${GREEN}↑${BASH_REMATCH[1]}${NC} ${RED}↓${BASH_REMATCH[2]}${NC}"
            elif [[ "$branch_line" =~ \[ahead\ ([0-9]+)\] ]]; then
                branch_info+=" ${GREEN}↑${BASH_REMATCH[1]}${NC}"
            elif [[ "$branch_line" =~ \[behind\ ([0-9]+)\] ]]; then
                branch_info+=" ${RED}↓${BASH_REMATCH[1]}${NC}"
            else
                branch_info+=" ${GREEN}✓${NC}"
            fi
        elif [[ "$branch_line" =~ ^##\ (.+) ]]; then
            branch_info="${CYAN}${BASH_REMATCH[1]}${NC} (no remote)"
        else
            branch_info="${YELLOW}detached${NC}"
        fi

        # Get file changes (lines after the branch line)
        local file_changes
        file_changes=$(echo "$status_output" | tail -n +2)
        local has_changes=false
        [ -n "$file_changes" ] && has_changes=true

        local result=""
        if $has_changes; then
            # Count change types
            local staged=0 modified=0 untracked=0
            staged=$(echo "$file_changes" | grep -c '^[MADRC]' 2>/dev/null || true)
            modified=$(echo "$file_changes" | grep -c '^ [MDRC]\|^MM' 2>/dev/null || true)
            untracked=$(echo "$file_changes" | grep -c '^??' 2>/dev/null || true)

            result+="${YELLOW}${sep}${NC} ${YELLOW}$repo_name${NC} $branch_info\n"
            local summary=""
            [ "$staged" -gt 0 ] && summary+="${GREEN}${staged} staged${NC}  "
            [ "$modified" -gt 0 ] && summary+="${RED}${modified} modified${NC}  "
            [ "$untracked" -gt 0 ] && summary+="${YELLOW}${untracked} untracked${NC}  "
            result+="  $summary\n"
            while IFS= read -r line; do
                result+="  $line\n"
            done <<< "$file_changes"
            dirty_output+="$result"
            ((dirty_count++))
        else
            result+="${GREEN}${sep}${NC} ${GREEN}$repo_name${NC} $branch_info\n"
            clean_output+="$result"
            ((clean_count++))
        fi
        ((total_count++))
    }

    # Find all .git directories and process their parent folders
    while IFS= read -r gitdir; do
        local repo_dir="${gitdir%/.git}"
        local repo_name="${repo_dir#$start_dir/}"
        [ "$repo_dir" = "$start_dir" ] && repo_name="$(basename "$repo_dir")"
        _gitsAll_format_repo "$repo_dir" "$repo_name"
        found_any=true
    done < <(find "$start_dir" -name ".git" -type d 2>/dev/null | sort)

    if ! $found_any; then
        echo "No git repositories found under $(realpath "$start_dir")"
        return
    fi

    # Print clean repos first, then dirty
    [ -n "$clean_output" ] && echo -ne "$clean_output"
    [ -n "$dirty_output" ] && echo -ne "$dirty_output"

    # Summary
    local summary="${GREEN}${clean_count} clean${NC}"
    [ "$dirty_count" -gt 0 ] && summary+=", ${YELLOW}${dirty_count} dirty${NC}"
    echo -e "${sep}\n${summary}"
}
export -f gitsAll

gitCommitsToDirs() {
    local start_dir="$(pwd)"
    local url="$1"
    local branch="${2:-main}"
    [ -z "$url" ] && return 1
    local repo
    repo="$(basename "$url" .git)"
    git clone --no-checkout --branch "$branch" "$url" "$repo" || return 1
    cd "$repo" || return 1
    mapfile -t commits < <(git rev-list --reverse "$branch")
    local i=1
    for c in "${commits[@]}"; do
        git worktree add "../${repo}_${i}" "$c"
        i=$((i+1))
    done
    cd "$start_dir"
}

gitInfo() {
    if ! git rev-parse --is-inside-work-tree &>/dev/null; then
        echo "Not a git repository"
        return 1
    fi

    local branch remote_url last_commit_hash last_commit_msg last_commit_date
    local total_commits behind ahead

    branch=$(git branch --show-current 2>/dev/null)
    [ -z "$branch" ] && branch="(detached HEAD)"

    remote_url=$(git remote get-url origin 2>/dev/null || echo "(no remote)")

    last_commit_hash=$(git rev-parse --short HEAD 2>/dev/null)
    last_commit_msg=$(git log -1 --pretty=format:'%s' 2>/dev/null)
    last_commit_date=$(git log -1 --pretty=format:'%ad' --date=format:'%Y-%m-%d %H:%M' 2>/dev/null)

    total_commits=$(git rev-list --count HEAD 2>/dev/null)

    # Fetch and check ahead/behind
    git fetch origin "$branch" &>/dev/null 2>&1
    local counts
    counts=$(git rev-list --left-right --count "origin/$branch...HEAD" 2>/dev/null)
    if [ -n "$counts" ]; then
        behind=$(echo "$counts" | cut -f1)
        ahead=$(echo "$counts" | cut -f2)
    else
        behind="-"
        ahead="-"
    fi

    echo -e "${YELLOW}Branch:${NC}       $branch"
    echo -e "${YELLOW}Remote:${NC}       $remote_url"
    echo -e "${YELLOW}Last commit:${NC}  $last_commit_hash - $last_commit_msg"
    echo -e "${YELLOW}Date:${NC}         $last_commit_date"
    echo -e "${YELLOW}Total:${NC}        $total_commits commits"
    echo -e "${YELLOW}Ahead/Behind:${NC} +$ahead / -$behind"
}
export -f gitInfo

gitClone(){
    if [ -z "$1" ]; then
        echo "Usage: gitClone <repo_name>"
        return 1
    fi
    git clone "https://github.com/yanivprusman/$1"
}
export -f gitClone
