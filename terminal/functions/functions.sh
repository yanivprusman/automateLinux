#!/bin/bash
# Functions module loader
# Organizes all shell functions into logical categories

. $(theRealPath dirHistory.sh)
. $(theRealPath emoji.sh)
. $(theRealPath terminal.sh)
. $(theRealPath files.sh)
. $(theRealPath daemon.sh)
. $(theRealPath variables.sh)
. $(theRealPath system.sh)
. $(theRealPath misc.sh)
. $(theRealPath build.sh)
# realPath build.sh
# echo $(realpath build.sh)
#  do not delete empty rows above this line