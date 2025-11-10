green='\e[32m'
yellow='\e[33m'
NC='\e[0m' # No Color
dirHistory_dev_pts=()
for f in ${AUTOMATE_LINUX_DIR_HISTORY_DIR}*; do
    [[ $(basename "$f") =~ ^dirHistory_ ]] && dirHistory_dev_pts+=("$f")
done
for file in "${dirHistory_dev_pts[@]}"; do
    echo -e "${green}$(basename "$file"):${NC}"
    cat "$file"
    echo -e "${yellow}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
done
echo -e "${green}$(basename "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"):${NC}"
cat "$AUTOMATE_LINUX_DIR_HISTORY_POINTERS_FILE"
echo -e "${yellow}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
