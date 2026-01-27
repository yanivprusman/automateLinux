#!/bin/bash
source /opt/automateLinux/terminal/functions/misc.sh

# Mock d function
d() {
    if [[ "$2" == "getKeyboardPath" ]]; then
        echo "/dev/input/kbd"
    elif [[ "$2" == "getMousePath" ]]; then
        echo "/dev/input/mouse"
    fi
}
export -f d

# Mock sudo evsieve
sudo() {
    echo "MOCK SUDO: $@"
}
export -f sudo

echo "Testing evsievep -k:"
evsievep -k

echo -e "\nTesting evsievep -k -m -p test:"
evsievep -k -m -p test
