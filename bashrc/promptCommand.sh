# pwd > "$AUTOMAT_LINUX_DIR_HISTORY_FILE"
line_no=$AUTOMAT_LINUX_DIR_HISTORY_POINTER
echo "$line_no"
current_dir=$(pwd)
if [ -s "$AUTOMAT_LINUX_DIR_HISTORY_FILE" ] && [ $(wc -l < "$AUTOMAT_LINUX_DIR_HISTORY_FILE") -ge "$line_no" ]; then
    sed -i "${line_no}s|.*|$current_dir|" "$AUTOMAT_LINUX_DIR_HISTORY_FILE"
else
    while [ $(wc -l < "$AUTOMAT_LINUX_DIR_HISTORY_FILE") -lt $((line_no-1)) ]; do
        echo "" >> "$AUTOMAT_LINUX_DIR_HISTORY_FILE"
    done
    echo "$current_dir" >> "$AUTOMAT_LINUX_DIR_HISTORY_FILE"
fi



