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
    # Replicate monitor settings
    if [ -f "$SOURCE_HOME/.config/monitors.xml" ]; then
        sudo cp "$SOURCE_HOME/.config/monitors.xml" "$TARGET_HOME/.config/monitors.xml"
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
    sudo groupadd -f "$NEW_USER"        
    sudo useradd -m -g "$NEW_USER" "$NEW_USER"
    echo "$NEW_USER:\\" | sudo chpasswd
    _theUserSetToBash "$NEW_USER"
    _theUserAddToCoding "$NEW_USER"
    sudo mkdir -p "$NEW_HOME"
    sudo chown -R "$NEW_USER:$NEW_USER" "$NEW_HOME"
    sudo chmod 755 "$NEW_HOME"
    sudo -u "$NEW_USER" mkdir -p "$NEW_HOME/.config"
    sudo -u "$NEW_USER" touch "$NEW_HOME/.config/gnome-initial-setup-done"
    __theUserReplicateGnome "$NEW_USER"
    sudo rm -rf "$NEW_HOME/.config/Code"
    sudo ln -s /home/yaniv/.config/Code "$NEW_HOME/.config/Code"
    sudo rm -rf "$NEW_HOME/.config/google-chrome"
    sudo ln -s /home/yaniv/.config/google-chrome "$NEW_HOME/.config/google-chrome"
    sudo rm -rf "$NEW_HOME/.vscode"
    sudo ln -s /home/yaniv/.vscode "$NEW_HOME/.vscode"
    sudo rm -rf "$NEW_HOME/coding"
    sudo ln -s /home/yaniv/coding "$NEW_HOME/coding"
    sudo rm -f "$NEW_HOME/.bashrc"
    sudo ln -s /home/yaniv/coding/automateLinux/terminal/bashrc "$NEW_HOME/.bashrc"
    sudo chown -h "$NEW_USER:$NEW_USER" "$NEW_HOME/.config/Code" "$NEW_HOME/.config/google-chrome" "$NEW_HOME/.vscode" "$NEW_HOME/coding" "$NEW_HOME/.bashrc"
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
    # Hardcode users you never want to delete here:
    local HARDCODED_USERS=(
        "yaniv"
        "test"
        # "userB"
    )
    
    local KEEP_USERS=("${HARDCODED_USERS[@]}" "$@")
    local PATTERN
    PATTERN="^($(IFS='|'; echo "${KEEP_USERS[*]}"))$"

    echo "Keeping protected users: ${KEEP_USERS[*]}"
    _tul | grep -vE "$PATTERN" | while read -r u; do
        echo "Removing $u..."
        _tur "$u"
    done

    sudo systemctl stop accounts-daemon
    sudo bash -c 'rm -rf /var/lib/AccountsService/users/*'
    sudo systemctl start accounts-daemon
}


_tus() {
    local SOURCE_USER="yaniv"
    local SHARED_GROUP="coding"
    local RECURSIVE=false
    
    if [[ "$1" == "-a" || "$1" == "--all" ]]; then
        RECURSIVE=true
    fi

    local DIRS_TO_SHARE=(
        "/home/$SOURCE_USER/.config/Code"
        "/home/$SOURCE_USER/.config/google-chrome"
        "/home/$SOURCE_USER/.vscode"
        "/home/$SOURCE_USER/coding"
    )

    # Fast: Ensure the parent .config is traversable
    local CONFIG_DIR="/home/$SOURCE_USER/.config"
    if [ -d "$CONFIG_DIR" ]; then
        echo "Ensuring $CONFIG_DIR is traversable for $SHARED_GROUP..."
        sudo setfacl -m "g:$SHARED_GROUP:rx" "$CONFIG_DIR"
    fi

    for dir in "${DIRS_TO_SHARE[@]}"; do
        if [ -d "$dir" ]; then
            if [ "$RECURSIVE" = true ]; then
                echo "Recursively reclaiming and setting up ACLs for $dir (this may take a while)..."
                sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$dir"
                sudo chmod -R g+rwx,g+s "$dir"
                sudo setfacl -R -b "$dir"
                sudo setfacl -R -m "u:$SOURCE_USER:rwx,g:$SHARED_GROUP:rwx,m:rwx" "$dir"
                sudo setfacl -R -d -m "u:$SOURCE_USER:rwx,g:$SHARED_GROUP:rwx,m:rwx" "$dir"
            else
                echo "Fast setup for $dir (parent directory and non-recursive ACLs)..."
                sudo chown "$SOURCE_USER:$SHARED_GROUP" "$dir"
                sudo chmod g+rwx,g+s "$dir"
                # Set mask and group permission on the directory itself (fast)
                sudo setfacl -m "g:$SHARED_GROUP:rwx,m:rwx" "$dir"
            fi
        else
            echo "Warning: Shared directory $dir does not exist. Skipping."
        fi
    done

    # Fast: Ensure daemon socket is accessible
    local SOCKET_DIR="/run/automatelinux"
    local SOCKET="$SOCKET_DIR/automatelinux-daemon.sock"
    if [ -d "$SOCKET_DIR" ]; then
        echo "Ensuring daemon socket directory is traversable..."
        sudo chown "$SOURCE_USER:$SHARED_GROUP" "$SOCKET_DIR"
        sudo chmod g+rws "$SOCKET_DIR"
        sudo setfacl -m "g:$SHARED_GROUP:rx,m:rwx" "$SOCKET_DIR"
    fi
    if [ -S "$SOCKET" ]; then
        echo "Setting up permissions for daemon socket (making it globally accessible)..."
        sudo chown "$SOURCE_USER:$SHARED_GROUP" "$SOCKET"
        sudo chmod 0666 "$SOCKET"
        sudo setfacl -m "g:$SHARED_GROUP:rw,m:rw,o:rw" "$SOCKET"
    fi

    if [ "$RECURSIVE" = false ]; then
        echo "💡 Tip: Run '_tus -a' if you need to fix permissions recursively (lengthy)."
    fi
}

_tuFixChrome() {
    local SOURCE_USER="yaniv"
    local SHARED_GROUP="coding"
    local DIR="/home/$SOURCE_USER/.config/google-chrome"
    if [ -d "$DIR" ]; then
        echo "Breaking the circle: Reclaiming Chrome profile in $DIR..."
        # 1. Delete all lock files (stale or from other users)
        sudo find "$DIR" -name "SingletonLock" -delete 2>/dev/null || true
        sudo find "$DIR" -name "LOCK" -delete 2>/dev/null || true
        # 2. Reclaim ownership to yaniv
        sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$DIR"
        # 3. Force explicit permissions (Directories: 2770, Files: 660)
        sudo find "$DIR" -type d -exec chmod 2770 {} +
        sudo find "$DIR" -type f -exec chmod 660 {} +
        # 4. Force the ACL mask to rwx for group consistency
        sudo setfacl -R -m "m:rwx" "$DIR"
        echo "Done. Chrome should now open clearly for you."
    fi
}
_tuBackupChromeProfile() {
    local BACKUP_DIR="/home/yaniv/.config/google-chrome-backup"
    sudo mkdir -p "$BACKUP_DIR"
    echo "Backing up Chrome profile to $BACKUP_DIR..."
    sudo cp -r "/home/yaniv/.config/google-chrome/Default" "$BACKUP_DIR/"
    sudo chown -R yaniv:coding "$BACKUP_DIR"
    echo "Backup complete."
}

_tuRestoreChromeProfile() {
    local BACKUP_DIR="/home/yaniv/.config/google-chrome-backup/Default"
    if [ ! -d "$BACKUP_DIR" ]; then
        echo "Error: Backup not found at $BACKUP_DIR"
        return 1
    fi
    echo "Restoring Chrome profile from $BACKUP_DIR..."
    sudo rm -rf "/home/yaniv/.config/google-chrome/Default"
    sudo cp -r "$BACKUP_DIR" "/home/yaniv/.config/google-chrome/"
    _tus
    echo "Restore complete. Please restart Chrome."
}
