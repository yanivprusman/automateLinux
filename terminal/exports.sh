export FREECAD_MACROS_DIR="/home/yaniv/coding/freeCad/Macros/"
EVSIEVE_LOG_FILE=$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/../evsieve/log/log.txt")
export KEYBOARD_BY_ID=$(ls /dev/input/by-id/ | grep 'Corsair.*-event-kbd')
