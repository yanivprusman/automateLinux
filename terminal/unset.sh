unset file 
unset myBashSourceFiles
b=()
for var in $(compgen -v); do
    if [[ ! " ${varsIAdded[@]} " =~ " $var " ]]; then
        b+=("$var")
    fi
done
remove=(varsIAdded b var )
b=($(printf "%s\n" "${b[@]}" | grep -vxF -f <(printf "%s\n" "${remove[@]}")))
# echo ${b[@]} | tr " " "\n" | sort
unset var varsIAdded b remove
