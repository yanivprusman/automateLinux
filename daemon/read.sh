exec {fd}< src/main.cpp    # dynamic FD allocation
echo "File Descriptor: $fd"
for i in {1..2440}; do
    read -u $fd line
    echo "Line $i: $line"
done
# read -u $fd line
# echo "Line: $line"

exec {fd}<&-            # close the FD