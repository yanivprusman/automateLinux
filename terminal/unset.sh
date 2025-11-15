unset file 
unset myBashSourceFiles
# varsFromMyBashrc=()
# for var in $(compgen -v); do
#     echo "$var"
#     if [[ ! " ${varsUntilMyBashrc[@]} " =~ " $var " ]]; then
#         varsFromMyBashrc+=("$var")
#     fi
# done
# remove=(varsUntilMyBashrc varsFromMyBashrc var )
# varsFromMyBashrc=($(printf "%s\n" "${varsFromMyBashrc[@]}" | grep -vxF -f <(printf "%s\n" "${remove[@]}")))
# echo ${varsFromMyBashrc[@]} | tr " " "\n" | sort
unset var varsUntilMyBashrc varsFromMyBashrc remove