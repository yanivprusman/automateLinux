deploy() {
    local app_name="$1"
    
    if [ -z "$app_name" ]; then
        echo "Usage: deploy <app_name>"
        echo "Supported apps: cad, pt (publicTransportation)"
        return 1
    fi

    local prod_base="$HOME/coding/prod"
    local app_dir=""

    # Map aliases to directory names
    case "$app_name" in
        "pt")
            app_dir="publicTransportation"
            ;;
        *)
            app_dir="$app_name"
            ;;
    esac

    local target_path="$prod_base/$app_dir"

    if [ ! -d "$target_path" ]; then
        echo -e "${RED}Error: Production directory not found: $target_path${NC}"
        return 1
    fi

    echo -e "${YELLOW}Deploying $app_name to production...${NC}"
    
    # Store current dir
    local original_dir=$(pwd)

    cd "$target_path" || return 1

    # Check if it's a git repo
    if [ -d ".git" ]; then
        echo -e "${BLUE}Pulling latest changes...${NC}"
        if ! git pull; then
            echo -e "${RED}Error: Git pull failed.${NC}"
            cd "$original_dir"
            return 1
        fi
    else
        echo -e "${YELLOW}Warning: Not a git repository, skipping pull.${NC}"
    fi

    # Run deploy script if it exists
    if [ -f "./deploy.sh" ]; then
        echo -e "${BLUE}Running deploy script...${NC}"
        # Ensure it's executable
        chmod +x ./deploy.sh
        if ./deploy.sh; then
            echo -e "${GREEN}Deployment successful!${NC}"
        else
            echo -e "${RED}Deployment script failed.${NC}"
            cd "$original_dir"
            return 1
        fi
    else
        echo -e "${YELLOW}No deploy.sh found in $target_path.${NC}"
    fi

    cd "$original_dir"
}

_deploy_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local apps="cad pt"
    COMPREPLY=($(compgen -W "$apps" -- "$cur"))
}

complete -F _deploy_completions deploy
