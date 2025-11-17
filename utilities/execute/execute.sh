outputTo5(){
    exec 1>/dev/pts/5
}
outputtoSelf(){
    exec 1>/dev/pts/$(tty | sed 's:/dev/pts/::')
}
move_and_print() {
    local x=$1
    local y=$2
    local z=$3
    # Move cursor: \033[y;xH
    echo -e "\033[${y};${x}H${z}"
}
get_cursor_position() {
    # Ask the terminal for cursor position
    # Save current stty settings
    oldstty=$(stty -g)
    # Disable echo and canonical mode
    stty -echo -icanon time 0 min 0
    # Request cursor position
    echo -en "\033[6n" > /dev/tty

    # Read the response: ESC [ rows ; cols R
    IFS=';' read -r -d R -a pos
    # Restore stty
    stty "$oldstty"

    # Extract row and column
    row=$(echo "${pos[0]}" | tr -d '[')
    col=${pos[1]}

    echo "$row $col"
}
cd ..
read row col <<< $(get_cursor_position)

move_and_print $col $row "Hello"
PS1=""
echo $PWD



