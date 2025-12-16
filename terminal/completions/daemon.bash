_daemon_completion() {
    local cur prev opts commands words
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    commands="(openedTty) (closedTty) (updateDirHistory) (cdForward) (cdBackward) showTerminalInstance showAllTerminalInstances deleteEntry showEntriesByPrefix deleteEntriesByPrefix showDB printDirHistory upsertEntry getEntry ping getKeyboardPath setKeyboard shouldLog toggleKeyboardsWhenActiveWindowChanges getDir getFile"
    opts="--help"
    keyboards="Code gnome-terminal-server google-chrome testKeyboard1 testKeyboard2"
    
    case "$prev" in
        daemon|d)
            COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
            ;;
        --json)
            COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
            ;;
        setKeyboard)
            COMPREPLY=( $(compgen -W "keyboardName=" -- "$cur") )
            ;;
        shouldLog|toggleKeyboardsWhenActiveWindowChanges)
            COMPREPLY=( $(compgen -W "enable=" -- "$cur") )
            ;;
        getDir)
            COMPREPLY=( $(compgen -W "dirName=" -- "$cur") )
            ;;
        getFile)
            COMPREPLY=( $(compgen -W "fileName=" -- "$cur") )
            ;;
        openedTty|updateDirHistory|cdForward|cdBackward|showTerminalInstance|deleteEntry|deleteEntriesByPrefix|showEntriesByPrefix|upsertEntry|getEntry|printDirHistory|ping|getKeyboardPath)
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            ;;
        *)
        case "$cur" in
            keyboardName=*)
                local keyboard_prefix="${cur#keyboardName=}"
                COMPREPLY=( $(compgen -W "$keyboards" -P "keyboardName=" -- "$keyboard_prefix") )
                ;;
            enable=*)
                local enable_prefix="${cur#enable=}"
                COMPREPLY=( $(compgen -W "true false" -P "enable=" -- "$enable_prefix") )
                ;;
            dirName=*)
                local dir_prefix="${cur#dirName=}"
                COMPREPLY=( $(compgen -W "base data mappings" -P "dirName=" -- "$dir_prefix") )
                ;;
            fileName=*)
                local file_prefix="${cur#fileName=}"
                local data_files="chrome.log combined.log daemon.db evsieveErr.log evsieveOutput.log trapErrLogBackground.txt trapErrLog.txt"
                local mapping_files="corsairKeyBoardLogiMouseCode.sh corsairKeyBoardLogiMouseDefaultKeyboard.sh corsairKeyBoardLogiMousegnome-terminal-server.sh corsairKeyBoardLogiMousegoogle-chrome.sh"
                COMPREPLY=( $(compgen -W "$data_files $mapping_files" -P "fileName=" -- "$file_prefix") )
                ;;
            -*)
                COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                ;;
            *)
                if [[ $COMP_CWORD -eq 1 || ( $COMP_CWORD -eq 2 && "${COMP_WORDS[1]}" == "--json" ) ]]; then
                    COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
                else
                    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                fi
                ;;
        esac
        ;;
    esac
}

complete -F _daemon_completion daemon d daemonAlternative
