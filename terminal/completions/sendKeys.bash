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
              keycode 
              period dot periodDown dotDown periodUp dotUp
              slash slashDown slashUp
              minus dash minusDown dashDown minusUp dashUp
              space spaceDown spaceUp
              comma commaDown commaUp
              equals equal equalsDown equalDown equalsUp equalUp
              semicolon semicolonDown semicolonUp
              apostrophe quote apostropheDown quoteDown apostropheUp quoteUp
              backslash backslashDown backslashUp
              bracket_left leftbracket bracket_leftDown leftbracketDown bracket_leftUp leftbracketUp
              bracket_right rightbracket bracket_rightDown rightbracketDown bracket_rightUp rightbracketUp
              backtick grave backtickDown graveDown backtickUp graveUp
              numlock numlockDown numlockUp
              enter enterDown enterUp
              syn
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
