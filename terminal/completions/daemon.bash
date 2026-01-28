_daemon_completion() {
    local cur prev cword pword
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    cword=${COMP_CWORD}

    pword_idx=$((cword - 1))
    
    # Function to get daemon commands (excluding the `send` itself)
    get_daemon_commands() {
        echo "(openedTty) (closedTty) (updateDirHistory) (cdForward) (cdBackward) showTerminalInstance showAllTerminalInstances deleteEntry showEntriesByPrefix deleteEntriesByPrefix showDB printDirHistory upsertEntry getEntry ping getKeyboardPath getMousePath getSocketPath setKeyboard enableKeyboard disableKeyboard getKeyboard getKeyboardEnabled shouldLog toggleKeyboard getDir getFile (activeWindowChanged) help quit simulateInput addLogFilter removeLogFilter listLogFilters clearLogFilters emptyDirHistoryTable isLoomActive restartLoom stopLoom generateLoomToken revokeLoomTokens publicTransportationStartProxy publicTransportationOpenApp listWindows activateWindow resetClock listPorts deletePort getPort setPort listCommands setPeerConfig getPeerStatus listPeers getPeerInfo claimApp releaseApp listApps getAppOwner updateNginxForward"
    }

    # Find the real command word and previous word, accounting for 'send'
    if [[ "${COMP_WORDS[1]}" == "send" ]]; then
        if [[ "${COMP_WORDS[pword_idx]}" == "send" ]]; then
            prev="" # No previous command word if 'send' was the immediate prior
        else
            prev="${COMP_WORDS[pword_idx]}"
        fi
        
        # If 'send' is the current word or previous, then the actual command will be at index 2 or 3
        if [[ ${cword} -eq 2 ]]; then # daemon send |
             COMPREPLY=( $(compgen -W "$(get_daemon_commands)" -- "$cur") )
             return 0
        elif [[ ${cword} -ge 3 ]]; then # daemon send command |
            command_idx=2
            if [[ "${COMP_WORDS[command_idx]}" == "send" ]]; then # In case send itself is the command
                command_idx=1
            fi
            local daemon_command="${COMP_WORDS[command_idx]}"
        fi
    else
        # If 'send' is omitted, the command starts at index 1
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        local daemon_command="${COMP_WORDS[1]}"
        
        # Suggest both 'send' and subcommands at index 1
        if [[ ${cword} -eq 1 ]]; then
            COMPREPLY=( $(compgen -W "send $(get_daemon_commands)" -- "$cur") )
            return 0
        fi
    fi

    # Define arguments for each command
    declare -A command_args
    command_args[(openedTty)]="--tty"
    command_args[(closedTty)]="--tty"
    command_args[(updateDirHistory)]="--tty --pwd"
    command_args[(cdForward)]="--tty"
    command_args[(cdBackward)]="--tty"
    command_args[showTerminalInstance]="--tty"
    command_args[deleteEntry]="--key"
    command_args[showEntriesByPrefix]="--prefix"
    command_args[deleteEntriesByPrefix]="--prefix"
    command_args[upsertEntry]="--key --value"
    command_args[getEntry]="--key"

    command_args[setKeyboard]="--windowTitle --wmClass --wmInstance --windowId"
    command_args[shouldLog]="--enable"
    command_args[toggleKeyboard]="--enable"
    command_args[getDir]="--dirName"
    command_args[getDir]="--dirName"
    command_args[getFile]="--fileName"
    command_args[simulateInput]="--string --type --code --value --key"
    command_args[getKeyboard]=""
    command_args[getKeyboardEnabled]=""
    command_args[getSocketPath]=""
    command_args[activeWindowChanged]=""
    command_args[help]="--help"
    command_args[addLogFilter]="--type --code --value --devicePathRegex --isKeyboard --action"
    command_args[removeLogFilter]="--type --code --value --devicePathRegex --isKeyboard"
    command_args[listLogFilters]=""
    command_args[clearLogFilters]=""
    command_args[emptyDirHistoryTable]=""
    command_args[isLoomActive]=""
    command_args[restartLoom]=""
    command_args[stopLoom]=""
    command_args[generateLoomToken]=""
    command_args[revokeLoomTokens]=""
    command_args[publicTransportationStartProxy]=""
    command_args[publicTransportationOpenApp]=""
    command_args[listWindows]=""
    command_args[activateWindow]="--windowId"
    command_args[resetClock]=""
    command_args[listPorts]=""
    command_args[deletePort]="--key"
    command_args[getPort]="--key"
    command_args[setPort]="--key --value"

    # Peer networking commands
    command_args[setPeerConfig]="--role --id --leader"
    command_args[getPeerStatus]=""
    command_args[listPeers]=""
    command_args[getPeerInfo]="--peer"
    command_args[claimApp]="--app"
    command_args[releaseApp]="--app"
    command_args[listApps]=""
    command_args[getAppOwner]="--app"
    command_args[updateNginxForward]="--port --target"

    # Define possible values for specific arguments
    declare -A arg_values
    arg_values[--enable]="true false"
    arg_values[--dirName]="base data mappings"
    arg_values[--fileName]="chrome.log combined.log daemon.db evsieveErr.log evsieveOutput.log trapErrLogBackground.txt trapErrLog.txt corsairKeyBoardLogiMouseCode.sh corsairKeyBoardLogiMouseDefaultKeyboard.sh corsairKeyBoardLogiMousegnome-terminal-server.sh corsairKeyBoardLogiMouseAll.sh corsairKeyBoardLogiMousegoogle-chrome.sh theRealPath.sh"
    # Placeholder for --tty and --pwd, could be dynamic later
    arg_values[--tty]="" 
    arg_values[--pwd]=""
    arg_values[--key]="keyA keyB keyC keyD keyE keyF keyG keyH keyI keyJ keyK keyL keyM keyN keyO keyP keyQ keyR keyS keyT keyU keyV keyW keyX keyY keyZ keyADown keyBDown keyCDown keyDDown keyEDown keyFDown keyGDown keyHDown keyIDown keyJDown keyKDown keyLDown keyMDown keyNDown keyODown keyPDown keyQDown keyRDown keySDown keyTDown keyUDown keyVDown keyWDown keyXDown keyYDown keyZDown keyAUp keyBUp keyCUp keyDUp keyEUp keyFUp keyGUp keyHUp keyIUp keyJUp keyKUp keyLUp keyMUp keyNUp keyOUp keyPUp keyQUp keyRUp keySUp keyTUp keyUUp keyVUp keyWUp keyXUp keyYUp keyZUp period dot periodDown dotDown periodUp dotUp slash slashDown slashUp minus dash minusDown dashDown minusUp dashUp space spaceDown spaceUp comma commaDown commaUp equals equal equalsDown equalDown equalsUp equalUp backspace backspaceDown backspaceUp semicolon semicolonDown semicolonUp apostrophe quote apostropheDown quoteDown apostropheUp quoteUp backslash backslashDown backslashUp bracket_left leftbracket bracket_leftDown leftbracketDown bracket_leftUp leftbracketUp bracket_right rightbracket bracket_rightDown rightbracketDown bracket_rightUp rightbracketUp backtick grave backtickDown graveDown backtickUp graveUp numlock numlockDown numlockUp enter enterDown enterUp syn"
    arg_values[--value]=""
    arg_values[--prefix]=""
    arg_values[--action]="show hide"
    arg_values[--isKeyboard]="true false"
    arg_values[--type]="0 1 2 3 4 5 17 20 21" # Common EV_ types for now
    arg_values[--code]="" # Can be dynamically fetched or extensive, leave blank for now
    arg_values[--value]="" # Leave blank for now
    arg_values[--devicePathRegex]="" # Leave blank for now

    # Peer networking argument values
    arg_values[--role]="leader worker"
    arg_values[--app]="cad loom pt automateLinux"
    arg_values[--id]="" # Custom peer ID
    arg_values[--leader]="" # Leader IP address
    arg_values[--peer]="" # Peer ID to query
    arg_values[--target]="" # Target IP for nginx forwarding
    arg_values[--port]="" # Port number


    # If completing a command argument
    if [[ "$cur" == --* ]]; then
        # Suggest arguments for the current command
        local possible_args="${command_args[$daemon_command]}"
        COMPREPLY=( $(compgen -W "$possible_args" -- "$cur") )
        return 0
    fi

    # If completing argument value
    # Find the argument key that 'prev' refers to
    local arg_key_for_value=""
    local search_start_idx=2
    if [[ "${COMP_WORDS[1]}" != "send" ]]; then
        search_start_idx=1
    fi

    for word_idx in $(seq $search_start_idx $((cword-1))); do
        if [[ "${COMP_WORDS[word_idx]}" == --* ]]; then
            arg_key_for_value="${COMP_WORDS[word_idx]}"
        fi
    done

    if [[ -n "$arg_key_for_value" ]]; then
        local values="${arg_values[$arg_key_for_value]}"
        if [[ -n "$values" ]]; then
            COMPREPLY=( $(compgen -W "$values" -- "$cur") )
            return 0
        fi
    fi

    # Default completion for values (e.g., tty, pwd) - suggest files/dirs
    if [[ "$prev" == --tty || "$prev" == --pwd ]]; then
         _filedir
         return 0
    fi
    
    # If no specific argument completion, suggest command arguments as a fallback
    local possible_args="${command_args[$daemon_command]}"
    COMPREPLY=( $(compgen -W "$possible_args" -- "$cur") )
    return 0
}

complete -F _daemon_completion daemon d
