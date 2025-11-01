#!/bin/bash

# Source the theRealPath function
. "$(dirname "$0")/theRealPath.sh"

echo "Current script path: ${BASH_SOURCE[0]}"
echo "Resolving _sudoStop.sh: $(theRealPath _sudoStop.sh)"
echo "Resolving ../other.sh: $(theRealPath ../other.sh)"
echo "Resolving ./local.sh: $(theRealPath ./local.sh)"