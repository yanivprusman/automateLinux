#!/usr/bin/env bash

_sendkeys_completion() {
    local cur prev opts commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # List of options and commands
    opts="-h --help -k --keyboard"
    commands="keyA keyB keyC keyD keyE keyF keyG keyH keyI keyJ keyK keyL keyM keyN keyO keyP keyQ keyR keyS keyT keyU keyV keyW keyX keyY keyZ
              keyADown keyBDown keyCDown keyDDown keyEDown keyFDown keyGDown keyHDown keyIDown keyJDown keyKDown keyLDown keyMDown keyNDown keyODown keyPDown keyQDown keyRDown keySDown keyTDown keyUDown keyVDown keyWDown keyXDown keyYDown keyZDown
              keyAUp keyBUp keyCUp keyDUp keyEUp keyFUp keyGUp keyHUp keyIUp keyJUp keyKUp keyLUp keyMUp keyNUp keyOUp keyPUp keyQUp keyRUp keySUp keyTUp keyUUp keyVUp keyWUp keyXUp keyYUp keyZUp
              keycode period dot slash minus dash space comma equals equal semicolon apostrophe quote backslash bracket_left leftbracket bracket_right rightbracket backtick grave
              numlock enter syn
              Code gnome-terminal-server google-chrome"

    # Handle option arguments
    case "${prev}" in
        -k|--keyboard)
            # Complete with device paths from /dev/input/by-id/
            COMPREPLY=( $(compgen -W "$(ls /dev/input/by-id/ 2>/dev/null)" -- ${cur}) )
            return 0
            ;;
    esac

    # If current word starts with -, complete from opts
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi

    # Otherwise complete from commands
    COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
    return 0
}

# Complete both the command name and the full path
complete -F _sendkeys_completion sendKeys
complete -F _sendkeys_completion ./toggle/sendKeys
