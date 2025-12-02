# Variable and array utilities

showArrays() {
    for array in $(compgen -A arrayvar); do
        echo -e "${green}$array${nc}"
        eval echo "\${${array}[@]}"
        echo "----"
    done
}
export -f showArrays

showVars() {
    for var in $(compgen -v); do
        echo -e "${green}$var${NC} = ${!var}"
    done
}
export -f showVars

printArray() {
    local arr_name=$1
    local keys
    eval "keys=(\"\${!${arr_name}[@]}\")"
    for key in "${keys[@]}"; do
        eval "echo \"$key='\${${arr_name}[${key}]}'\""
    done
}
export -f printArray

deleteFunctions() {
    for f in $(declare -F | awk '{print $3}'); do
        unset -f "$f"
    done
}
export -f deleteFunctions

isSet() {
    local var_name="$1"
    if [ -z "${!var_name+x}" ]; then
        echo false
        return 1
    else
        echo true
        return 0
    fi
}

