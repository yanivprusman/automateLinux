green='\033[0;32m'
nc='\033[0m' # No Color
for array in $(compgen -A arrayvar); do
    echo -e "${green}$array${nc}"
    eval echo "\${${array}[@]}"
    echo "----"
done
