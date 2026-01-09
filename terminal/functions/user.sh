_tul(){
    awk -F: '$3 >= 1000 && $3 < 60000 {print $1}' /etc/passwd
}

_theUserSetToBash(){
    sudo usermod -s /bin/bash $1
}

_theUserAddToCoding(){
    sudo usermod -aG coding $1
}

__theUserReplicateGnome(){
    local NEW_USER="$1"
    local SOURCE_USER="yaniv"
    local SOURCE_HOME="/home/$SOURCE_USER"
    local TARGET_HOME="/home/$NEW_USER"
    echo "Replicating GNOME settings from $SOURCE_USER to $NEW_USER..."
    sudo -u "$SOURCE_USER" dconf dump / > /tmp/yaniv-settings.conf
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf load / < /tmp/yaniv-settings.conf
    rm /tmp/yaniv-settings.conf
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf write /org/gnome/shell/welcome-dialog-last-shown-version "'99.9'"
    local FAVORITE_APPS="['code.desktop', 'google-chrome.desktop', 'libreoffice-writer.desktop', 'org.gnome.Terminal.desktop', 'com.mattjakeman.ExtensionManager.desktop', 'freecad.desktop', 'org.gnome.GPaste.Ui.desktop', 'barrier.desktop', 'discord_discord.desktop', 'thunderbird_thunderbird.desktop', 'org.gnome.Nautilus.desktop', 'snap-store_snap-store.desktop', 'yelp.desktop', 'org.gnome.Evince.desktop', 'org.gnome.Settings.desktop', 'org.gnome.Calculator.desktop', 'postman_postman.desktop', 'tableplus.desktop', 'org.gnome.Extensions.desktop', 'org.kde.krita.desktop', 'realvnc-vncviewer.desktop']"
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf write /org/gnome/shell/favorite-apps "$FAVORITE_APPS"
    for ver in 3.0 4.0; do
        if [ -d "$SOURCE_HOME/.config/gtk-$ver" ]; then
            sudo mkdir -p "$TARGET_HOME/.config/gtk-$ver"
            sudo cp -r "$SOURCE_HOME/.config/gtk-$ver/"* "$TARGET_HOME/.config/gtk-$ver/" 2>/dev/null || true
        fi
    done
    if [ -d "$SOURCE_HOME/.local/share/gnome-shell/extensions" ]; then
        sudo mkdir -p "$TARGET_HOME/.local/share/gnome-shell/extensions"
        sudo cp -r "$SOURCE_HOME/.local/share/gnome-shell/extensions/"* "$TARGET_HOME/.local/share/gnome-shell/extensions/" 2>/dev/null || true
    fi
    sudo chown -R "$NEW_USER:$NEW_USER" "$TARGET_HOME"
}

_setGidSameAsUid() {
    local user="$1"
    local uid gid
    uid=$(id -u "$user") || { echo "User not found"; return 1; }
    gid=$uid
    if ! getent group "$gid" >/dev/null; then
        sudo groupadd -g "$gid" "$user"
    fi
    sudo usermod -g "$gid" "$user"
    echo "Set primary GID of $user to $gid"
}



_tuc(){
    local NEW_USER="${1:-$(password)}"
    local SOURCE_USER="yaniv"
    local NEW_HOME="/home/$NEW_USER"

    # Create user with primary group having the same GID as UID
    sudo groupadd -f "$NEW_USER"        # ensure group exists
    sudo useradd -m -g "$NEW_USER" "$NEW_USER"
    echo "$NEW_USER:\\" | sudo chpasswd

    # Give bash shell and add to coding group
    _theUserSetToBash "$NEW_USER"
    _theUserAddToCoding "$NEW_USER"

    # Ensure home directory exists
    sudo mkdir -p "$NEW_HOME"
    sudo chown -R "$NEW_USER:$NEW_USER" "$NEW_HOME"
    sudo chmod 755 "$NEW_HOME"

    # Prepare GNOME config
    sudo -u "$NEW_USER" mkdir -p "$NEW_HOME/.config"
    sudo -u "$NEW_USER" touch "$NEW_HOME/.config/gnome-initial-setup-done"

    # Replicate GNOME settings
    __theUserReplicateGnome "$NEW_USER"

    # Link Code, Chrome and VSCode config with proper ownership
    sudo rm -rf "$NEW_HOME/.config/Code"
    sudo ln -s /home/yaniv/.config/Code "$NEW_HOME/.config/Code"

    sudo rm -rf "$NEW_HOME/.config/google-chrome"
    sudo ln -s /home/yaniv/.config/google-chrome "$NEW_HOME/.config/google-chrome"

    sudo rm -rf "$NEW_HOME/.vscode"
    sudo ln -s /home/yaniv/.vscode "$NEW_HOME/.vscode"

    sudo chown -h "$NEW_USER:$NEW_USER" "$NEW_HOME/.config/Code" "$NEW_HOME/.config/google-chrome" "$NEW_HOME/.vscode"
    sudo chown -R "$NEW_USER:$NEW_USER" "$NEW_HOME"
}

_tur(){
    sudo pkill -9 -u $1 || true
    sudo deluser --remove-home $1
    sudo rm -rf "/home/$1"
}

_theUserGroups(){
    groups $1
}

_theUserSwitch(){
    su - $1
}

_tuk(){
    # Keep only yaniv
    _tul | grep -v '^yaniv$' | while read -r u; do
        _tur "$u"
    done
    sudo systemctl stop accounts-daemon
    sudo bash -c 'rm -rf /var/lib/AccountsService/users/*'
    sudo systemctl start accounts-daemon
}


_setupSharedDirs() {
    local SOURCE_USER="yaniv"
    local SHARED_GROUP="coding"
    local DIRS_TO_SHARE=(
        "/home/$SOURCE_USER/.config/Code"
        "/home/$SOURCE_USER/.config/google-chrome"
        "/home/$SOURCE_USER/.vscode"
    )
    # Ensure the parent .config is traversable
    local CONFIG_DIR="/home/$SOURCE_USER/.config"
    if [ -d "$CONFIG_DIR" ]; then
        echo "Ensuring $CONFIG_DIR is traversable for $SHARED_GROUP..."
        sudo setfacl -m "g:$SHARED_GROUP:rx" "$CONFIG_DIR"
    fi

    for dir in "${DIRS_TO_SHARE[@]}"; do
        if [ -d "$dir" ]; then
            echo "Setting up ACLs for $dir..."
            sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$dir"
            sudo chmod -R g+rwx,g+s "$dir"
            # Use ACLs to ensure the group has full access and new files inherit it
            sudo setfacl -R -m "g:$SHARED_GROUP:rwx" "$dir"
            sudo setfacl -R -d -m "g:$SHARED_GROUP:rwx" "$dir"
        else
            echo "Warning: Shared directory $dir does not exist. Skipping."
        fi
    done
}