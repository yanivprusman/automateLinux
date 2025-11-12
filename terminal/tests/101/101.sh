a=${BASH_LINENO[0]}
echo line no: $a
history -p !$a

# a=${BASH_LINENO[0]}
# for ((i=1; i<=a; i++)); do
#     # echo "BASH_LINENO: $i"
#     history -p "!$i"
# done
# history -p !60
