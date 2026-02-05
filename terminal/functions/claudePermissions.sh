testClaudePermissions() {
    local settings_file="$HOME/.claude/settings.json"
    if [[ ! -f "$settings_file" ]]; then
        echo "Settings file not found: $settings_file"
        return 1
    fi

    # Extract Bash patterns from settings.json
    local -a patterns=()
    while IFS= read -r line; do
        patterns+=("$line")
    done < <(grep -oP 'Bash\(\K[^)]+' "$settings_file")

    if [[ ${#patterns[@]} -eq 0 ]]; then
        echo "No Bash permission patterns found."
        return 1
    fi

    echo "=== Bash Permission Patterns ==="
    for p in "${patterns[@]}"; do
        echo "  Bash($p)"
    done
    echo ""

    # Non-destructive simple commands (should be ALLOWED)
    local -a simple_commands=(
        "ls"
        "ls -la"
        "ls -la /opt/automateLinux"
        "ls -la /tmp"
        "ls -R /opt"
        "echo"
        "echo hello"
        "echo \"hello world\""
        "cat /etc/os-release"
        "cat README.md"
        "head -20 file.txt"
        "tail -f /var/log/syslog"
        "tail -n 50 logfile"
        "grep -r pattern /opt"
        "grep -rn TODO src/"
        "rg pattern src/"
        "find . -name '*.py'"
        "wc -l file.txt"
        "wc"
        "sort output.txt"
        "uniq -c sorted.txt"
        "cut -d: -f1 /etc/passwd"
        "diff file1.txt file2.txt"
        "du -sh /opt"
        "df"
        "df -h"
        "file /usr/bin/ls"
        "stat README.md"
        "tree"
        "tree /opt/automateLinux/daemon"
        "realpath ./symlink"
        "readlink /usr/bin/python3"
        "basename /opt/automateLinux/daemon/src/main.cpp"
        "dirname /opt/automateLinux/daemon/src/main.cpp"
        "ps"
        "ps aux"
        "top -bn1"
        "whoami"
        "hostname"
        "uname -a"
        "pwd"
        "which python3"
        "env"
        "printenv HOME"
        "id"
        "date"
        "date +%Y-%m-%d"
        "uptime"
        "free -h"
        "lsb_release -a"
        "cd /tmp"
        "git status"
        "git status -s"
        "git diff"
        "git diff HEAD~1"
        "git diff --staged"
        "git log"
        "git log --oneline -10"
        "git branch"
        "git branch -a"
        "git show HEAD"
        "git stash list"
        "git remote -v"
        "gh pr list"
        "gh issue list"
    )

    # Compound commands — Claude Code ALWAYS prompts for these
    # regardless of permission patterns (shell operator detection)
    local -a compound_commands=(
        "ls -la /tmp; echo done"
        "echo start; ls -la; echo end"
        "ls -la && echo success"
        "git status && git diff"
        "ls | grep foo"
        "ps aux | grep daemon"
        "grep pattern file || echo not found"
        "cat file | sort | uniq -c"
        "date; uptime"
        "git log --oneline | head -5"
    )

    # Check if command contains shell operators (;  &&  ||  |)
    has_shell_ops() {
        local cmd="$1"
        if [[ "$cmd" =~ \;|\&\&|\|\||\| ]]; then
            return 0
        fi
        return 1
    }

    # Test glob match using bash pattern matching
    matches_pattern() {
        local cmd="$1"
        local pattern="$2"
        [[ "$cmd" == $pattern ]]
    }

    check_command() {
        local cmd="$1"
        for pattern in "${patterns[@]}"; do
            if matches_pattern "$cmd" "$pattern"; then
                echo "ALLOWED:$pattern"
                return
            fi
        done
        echo "DENIED"
    }

    local allowed=0 denied=0 blocked=0 errors=0

    printf "\e[1m%-72s %s\e[0m\n" "COMMAND" "RESULT"
    printf "%0.s-" {1..95}; echo ""

    # Test simple commands (expect ALLOWED)
    echo ""
    echo -e "\e[1m  SIMPLE COMMANDS (expect: ALLOWED)\e[0m"
    for cmd in "${simple_commands[@]}"; do
        local result
        result=$(check_command "$cmd")
        if [[ "$result" == ALLOWED:* ]]; then
            local pat="${result#ALLOWED:}"
            printf "\e[32m  %-70s ALLOWED  [%s]\e[0m\n" "$cmd" "$pat"
            ((allowed++))
        else
            printf "\e[31m  %-70s DENIED   ← MISSING PATTERN\e[0m\n" "$cmd"
            ((denied++))
            ((errors++))
        fi
    done

    # Test compound commands (always blocked by Claude Code)
    echo ""
    echo -e "\e[1m  COMPOUND COMMANDS (always prompted — shell operators blocked)\e[0m"
    for cmd in "${compound_commands[@]}"; do
        if has_shell_ops "$cmd"; then
            printf "\e[33m  %-70s BLOCKED  (;  &&  ||  |)\e[0m\n" "$cmd"
            ((blocked++))
        else
            printf "\e[31m  %-70s NOT BLOCKED?\e[0m\n" "$cmd"
            ((errors++))
        fi
    done

    local total=$(( ${#simple_commands[@]} + ${#compound_commands[@]} ))
    echo ""
    echo "=== Summary ==="
    echo "  Total:   $total"
    echo "  Allowed: $allowed (simple, matched pattern)"
    echo "  Blocked: $blocked (compound, shell operators — always prompted)"
    echo "  Denied:  $denied (simple, no matching pattern)"
    if [[ $errors -gt 0 ]]; then
        echo -e "  \e[31mErrors:  $errors (see ← markers above)\e[0m"
    else
        echo -e "  \e[32mNo errors.\e[0m"
    fi
    echo ""
    echo "Note: Compound commands with ; && || | are ALWAYS prompted by Claude Code."
    echo "This is a security feature and cannot be overridden via permission patterns."
}
export -f testClaudePermissions
