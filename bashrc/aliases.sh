alias mk='mkdir'
alias boot='history -a && sudo reboot'
alias pkill='sudo pkill -9'
alias pkillev='sudo pkill -9 evsieve'
alias evsievep='sudo evsieve --input /dev/input/event* --print format=direct'
alias 1restart='restartCorsairKeyBoardLogiMouseService.sh reset'
alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'
alias trr='tr ":" "\n"'
alias sourceb='source ~/.bashrc'
alias lstr='ls --color=always | tr " " "\n"'
alias cls='clear'
alias cd..='cd ..'
alias cdc='cd /home/yaniv/coding/'
PS1_SCRIPT="$(dirname "${BASH_SOURCE[0]}")/ps1.sh"
PROMPT_COMMAND='PS1=$(bash "'"$PS1_SCRIPT"'")'
resetPromptColor() {
    echo -en "\033[00m"
}
trap 'resetPromptColor' DEBUG

