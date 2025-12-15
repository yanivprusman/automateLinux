#!/bin/bash
localFunction(){
    local script_name=$(basename ${SUDO_COMMAND%% *} )
    if [[ "$script_name" == "print" ]]; then
        # source ../functions/printDir.sh
        . $(theRealPath ../functions/printDir.sh )
        print $@
    else
        :
        # source ../functions/"$script_name".sh
    fi
}
localFunction "$@"
unset localFunction
# . $(theRealPath ../functions/printDir.sh -debug)
# print $@

# theRealPath