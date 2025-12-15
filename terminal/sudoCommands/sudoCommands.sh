#!/bin/bash
localFunction(){
    local script_name=$(basename ${SUDO_COMMAND%% *} )
    if [[ "$script_name" == "print" ]]; then
        # source ../functions/printDir.sh
        . $(theRealPath ../functions/printDir.sh )
        print $@
    else
        #. $(theRealPath "../functions/$script_name.sh" )
        #"$script_name" $@
        # theRealPath ../functions/$script_name.sh
        # theRealPath ../functions/printDir.sh
        # echo asdf
    fi
}
localFunction "$@"
unset localFunction
