#!/usr/bin/env bash
# r1=5; g1=5; b1=5    # white
# r2=5; g2=5; b2=5    # white
# setStart() {
#     rgb="$1"
#     r1="${rgb:0:1}"
#     g1="${rgb:1:1}"
#     b1="${rgb:2:1}"
# }
# setEnd() {
#     rgb="$1"
#     r2="${rgb:0:1}"
#     g2="${rgb:1:1}"
#     b2="${rgb:2:1}"
# }
# setStart 555
# setEnd 030
# setStart 030
# setEnd 555
# dir="${PWD/#$HOME/~}/"
# len=${#dir}
# prompt=""
# for ((i=0; i<len; i++)); do
#     f=$(( i * 100 / (len - 1) ))
#     r=$(( (r1 * (100 - f) + r2 * f) / 100 ))
#     g=$(( (g1 * (100 - f) + g2 * f) / 100 ))
#     b=$(( (b1 * (100 - f) + b2 * f) / 100 ))
#     color=$((16 + 36*r + 6*g + b))
#     char="${dir:$i:1}"
#     prompt+="\[\033[1;38;5;${color}m\]$char"
# done
# prompt+="\[\033[00m\]"
# echo "$prompt"

parseRGB() {
    local rgb="$1"
    echo "${rgb:0:1} ${rgb:1:1} ${rgb:2:1}"
}
setStart() {
    read r1 g1 b1 <<< "$(parseRGB "$1")"
}
setMiddle() {
    read r2 g2 b2 <<< "$(parseRGB "$1")"
}
setEnd() {
    read r3 g3 b3 <<< "$(parseRGB "$1")"
}
setStart 555    
setMiddle 030   
setEnd 555      
dir="${PWD/#$HOME/~}/"
len=${#dir}
prompt=""
for ((i=0; i<len; i++)); do
    if (( i < len/2 )); then
        # First half: start → middle
        f=$(( i * 100 / (len/2 - 1) ))
        r=$(( (r1*(100-f) + r2*f)/100 ))
        g=$(( (g1*(100-f) + g2*f)/100 ))
        b=$(( (b1*(100-f) + b2*f)/100 ))
    else
        # Second half: middle → end
        f=$(( (i - len/2) * 100 / (len - len/2 - 1) ))
        r=$(( (r2*(100-f) + r3*f)/100 ))
        g=$(( (g2*(100-f) + g3*f)/100 ))
        b=$(( (b2*(100-f) + b3*f)/100 ))
    fi
    color=$((16 + 36*r + 6*g + b))
    char="${dir:$i:1}"
    prompt+="\[\033[1;38;5;${color}m\]$char"
done
prompt+="\[\033[00m\]"
echo "$prompt"


