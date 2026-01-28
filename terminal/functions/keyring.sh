#!/bin/bash

# Keyring management functions

removeKeyringPassword() {
    echo -e "${CYAN}=== Remove GNOME Keyring Password ===${NC}"
    echo ""
    echo "This will remove the password from your login keyring so you won't"
    echo "get authentication prompts when opening VS Code or other apps."
    echo ""
    echo -e "${YELLOW}Steps:${NC}"
    echo "1. Look in the left sidebar for 'Passwords' and expand it"
    echo "2. Find 'Login' keyring (it has a lock icon)"
    echo "3. Right-click on 'Login' → select 'Change Password'"
    echo "4. Enter your current password (your user login password)"
    echo "5. Leave both new password fields empty (don't type anything)"
    echo "6. Click 'Continue'"
    echo "7. Confirm the security warning about unencrypted storage"
    echo ""
    echo -e "${GREEN}Opening Seahorse password manager...${NC}"

    # Check if seahorse is installed
    if ! command -v seahorse &> /dev/null; then
        echo -e "${RED}Seahorse is not installed. Installing...${NC}"
        sudo apt install -y seahorse
    fi

    # Launch seahorse
    seahorse &
    disown

    echo ""
    echo -e "${GREEN}Follow the steps above in the Seahorse window that just opened.${NC}"
}
