#!/bin/bash
echo $SUDO_COMMAND
$(SUDO_COMMAND) $@

# . $(theRealPath ../functions/printDir.sh -debug)
# print $@

# theRealPath