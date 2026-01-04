shedSwitch(){
    local target="$1"
    if [ -z "$target" ]; then
        echo "Usage: shedSwitch <commit_hash|branch_name>"
        return 1
    fi

    # Store current directory to return later
    local original_dir=$(pwd)
    local repo_dir="/home/yaniv/coding/cad"

    echo -e "${YELLOW}Switching shed version to: $target...${NC}"
    
    # Enter repo directory
    if [ ! -d "$repo_dir" ]; then
        echo -e "${RED}Error: Directory $repo_dir does not exist.${NC}"
        return 1
    fi
    cd "$repo_dir" || return 1

    # Checkout the target
    if ! git checkout "$target"; then
        echo -e "${RED}Error: Git checkout failed.${NC}"
        cd "$original_dir"
        return 1
    fi

    # Build the C++ backend
    echo -e "${YELLOW}Rebuilding C++ backend...${NC}"
    if ! ./build.sh; then
        echo -e "${RED}Error: Build failed.${NC}"
        cd "$original_dir"
        return 1
    fi

    echo -e "${GREEN}Success: Switched to $target and rebuilt backend.${NC}"
    git log -1 --oneline
    
    # Return to original directory
    cd "$original_dir"
}
export -f shedSwitch
