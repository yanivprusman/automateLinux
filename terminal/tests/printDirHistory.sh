dirHistory_dev_pts=()
for f in ${AUTOMATE_LINUX_DIR_HISTORY_DIR}*; do
    [[ $(basename "$f") =~ ^dirHistory_ ]] && dirHistory_dev_pts+=("$f")
done
for file in "${dirHistory_dev_pts[@]}"; do
    echo -e "${GREEN}$(basename "$file"):${NC}"
    cat "$file"
    echo -e "${YELLOW}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
done
echo -e "${GREEN}$"$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE":${NC}"
if [[ -f "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE" ]]; then
    cat "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
fi
echo -e "${YELLOW}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
