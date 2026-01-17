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
    
    # Replicate Desktop icons/files
    if [ -d "$SOURCE_HOME/Desktop" ]; then
        sudo cp -r "$SOURCE_HOME/Desktop" "$TARGET_HOME/"
    fi
    
    # Replicate GVFS metadata (Icon positions using 'home' metadata)
    if [ -d "$SOURCE_HOME/.local/share/gvfs-metadata" ]; then
        sudo mkdir -p "$TARGET_HOME/.local/share"
        sudo cp -r "$SOURCE_HOME/.local/share/gvfs-metadata" "$TARGET_HOME/.local/share/."
    fi

    # Replicate keyrings
    if [ -d "$SOURCE_HOME/.local/share/keyrings" ]; then
        sudo mkdir -p "$TARGET_HOME/.local/share"
        sudo cp -r "$SOURCE_HOME/.local/share/keyrings" "$TARGET_HOME/.local/share/."
    fi

    # Replicate Git settings
    if [ -f "$SOURCE_HOME/.gitconfig" ]; then
        sudo cp "$SOURCE_HOME/.gitconfig" "$TARGET_HOME/.gitconfig"
        sudo chown "$NEW_USER:$NEW_USER" "$TARGET_HOME/.gitconfig"
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
    local SOURCE_HOME="/home/$SOURCE_USER"
    local NEW_HOME="/home/$NEW_USER"
    
    # Find a free ID that is available as BOTH a UID and a GID
    local ID=1001
    while getent passwd $ID >/dev/null || getent group $ID >/dev/null; do
        ID=$((ID+1))
    done
    
    echo "Creating user $NEW_USER with strict UID=GID=$ID..."
    
    # Create the group with the specific ID
    sudo groupadd -g $ID "$NEW_USER"
    # Create the user with the matching UID and GID
    sudo useradd -u $ID -g $ID -m "$NEW_USER"
    
    echo "$NEW_USER:\\" | sudo chpasswd
    _theUserSetToBash "$NEW_USER"
    _theUserAddToCoding "$NEW_USER"
    
    # Replicate GNOME settings
    __theUserReplicateGnome "$NEW_USER"
    sudo -u "$NEW_USER" mkdir -p "$NEW_HOME/.config"
    sudo -u "$NEW_USER" touch "$NEW_HOME/.config/gnome-initial-setup-done"

    # Setup OverlayFS for application isolation
    # We store upper/work layers in a hidden .overlay folder in the new user's home
    local OVERLAY_BASE="$NEW_HOME/.overlay"
    local APPS=("google-chrome" "Code" ".vscode")
    
    for app in "${APPS[@]}"; do
        local LOWER_DIR="$SOURCE_HOME/.config/$app"
        [ "$app" == ".vscode" ] && LOWER_DIR="$SOURCE_HOME/$app"
        
        local TARGET_DIR="$NEW_HOME/.config/$app"
        [ "$app" == ".vscode" ] && TARGET_DIR="$NEW_HOME/$app"
        
        if [ -d "$LOWER_DIR" ]; then
            echo "Setting up OverlayFS for $app..."
            local UPPER="$OVERLAY_BASE/$app/upper"
            local WORK="$OVERLAY_BASE/$app/work"
            
            sudo -u "$NEW_USER" mkdir -p "$UPPER" "$WORK"
            sudo mkdir -p "$TARGET_DIR"
            
            # Mount the overlay
            sudo mount -t overlay overlay -o lowerdir="$LOWER_DIR",upperdir="$UPPER",workdir="$WORK" "$TARGET_DIR"
        fi
    done

    # Coding directory remains a direct symlink for collaboration
    sudo rm -rf "$NEW_HOME/coding"
    sudo ln -s "$SOURCE_HOME/coding" "$NEW_HOME/coding"
    
    # Shell configuration
    sudo rm -f "$NEW_HOME/.bashrc"
    sudo ln -s "$SOURCE_HOME/coding/automateLinux/terminal/bashrc" "$NEW_HOME/.bashrc"
    
    # Ensure GNOME autostart for GUI login persistence
    local AUTOSTART="$NEW_HOME/.config/autostart"
    sudo -u "$NEW_USER" mkdir -p "$AUTOSTART"
    # Using explicit user name to avoid $(whoami) evaluation at creation time
    echo -e "[Desktop Entry]\nType=Application\nName=Mount Overlays\nExec=bash -c 'source /home/yaniv/coding/automateLinux/terminal/functions/user.sh && _tuMountOverlays $NEW_USER'\nHidden=false\nNoDisplay=false\nX-GNOME-Autostart-enabled=true" | sudo -u "$NEW_USER" tee "$AUTOSTART/mount-overlays.desktop" >/dev/null

    # ALSO add to .profile for robust session initialization (before GUI starts)
    # This specifically addresses "cant launch chrome before opening terminal"
    echo "source /home/yaniv/coding/automateLinux/terminal/functions/user.sh" | sudo -u "$NEW_USER" tee -a "$NEW_HOME/.profile" >/dev/null
    echo "_tuMountOverlays $NEW_USER" | sudo -u "$NEW_USER" tee -a "$NEW_HOME/.profile" >/dev/null

    # Surgical ownership
    echo "Finalizing ownership (surgically)..."
    sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME"
    sudo chown -R "$NEW_USER:$NEW_USER" "$OVERLAY_BASE"
    sudo chown -h "$NEW_USER:$NEW_USER" "$NEW_HOME/coding" "$NEW_HOME/.bashrc"
    sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.config"
    
    # Ensure .gitconfig has correct ownership
    if [ -f "$NEW_HOME/.gitconfig" ]; then
        sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.gitconfig"
    fi

    # Replicate Git credentials
    if [ -f "$SOURCE_HOME/.git-credentials" ]; then
        sudo cp "$SOURCE_HOME/.git-credentials" "$NEW_HOME/.git-credentials"
        sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.git-credentials"
    fi
    
    # Fix masks on SOURCE so lowerdir is readable
    _tuFixConfig
}

_tucno(){
    local NEW_USER="${1:-$(password)}"
    local SOURCE_USER="yaniv"
    local SOURCE_HOME="/home/$SOURCE_USER"
    local NEW_HOME="/home/$NEW_USER"
    
    # Find a free ID that is available as BOTH a UID and a GID
    local ID=1001
    while getent passwd $ID >/dev/null || getent group $ID >/dev/null; do
        ID=$((ID+1))
    done
    
    echo "Creating user $NEW_USER with strict UID=GID=$ID (Shared Mode)..."
    
    # Create the group with the specific ID
    sudo groupadd -g $ID "$NEW_USER"
    # Create the user with the matching UID and GID
    sudo useradd -u $ID -g $ID -m "$NEW_USER"
    
    echo "$NEW_USER:\\" | sudo chpasswd
    _theUserSetToBash "$NEW_USER"
    _theUserAddToCoding "$NEW_USER"
    
    # Replicate GNOME settings
    __theUserReplicateGnome "$NEW_USER"
    sudo -u "$NEW_USER" mkdir -p "$NEW_HOME/.config"
    sudo -u "$NEW_USER" touch "$NEW_HOME/.config/gnome-initial-setup-done"

    # Symlink configs (Instead of OverlayFS)
    local APPS=("google-chrome" "Code" ".vscode")
    for app in "${APPS[@]}"; do
        local LOWER_DIR="$SOURCE_HOME/.config/$app"
        [ "$app" == ".vscode" ] && LOWER_DIR="$SOURCE_HOME/$app"
        
        local TARGET_DIR="$NEW_HOME/.config/$app"
        [ "$app" == ".vscode" ] && TARGET_DIR="$NEW_HOME/$app"
        
        if [ -d "$LOWER_DIR" ]; then
            echo "Symlinking $app for shared access..."
            sudo rm -rf "$TARGET_DIR"
            sudo ln -s "$LOWER_DIR" "$TARGET_DIR"
        fi
    done

    # Coding directory remains a direct symlink for collaboration
    sudo rm -rf "$NEW_HOME/coding"
    sudo ln -s "$SOURCE_HOME/coding" "$NEW_HOME/coding"
    
    # Shell configuration
    sudo rm -f "$NEW_HOME/.bashrc"
    sudo ln -s "$SOURCE_HOME/coding/automateLinux/terminal/bashrc" "$NEW_HOME/.bashrc"
    
    # Setup .profile for robust session initialization
    echo "source /home/yaniv/coding/automateLinux/terminal/functions/user.sh" | sudo -u "$NEW_USER" tee -a "$NEW_HOME/.profile" >/dev/null

    # Finalize ownership (surgically)
    echo "Finalizing ownership (surgically)..."
    sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME"
    sudo chown -h "$NEW_USER:$NEW_USER" "$NEW_HOME/coding" "$NEW_HOME/.bashrc"
    
    # Symlinks to .config/app should be owned by new user too
    # We use a more robust way to ensure we match the right paths
    for app in "google-chrome" "Code"; do
        local TARGET_DIR="$NEW_HOME/.config/$app"
        if [ -L "$TARGET_DIR" ]; then 
            sudo chown -h "$NEW_USER:$NEW_USER" "$TARGET_DIR"
        fi
    done
    if [ -L "$NEW_HOME/.vscode" ]; then
        sudo chown -h "$NEW_USER:$NEW_USER" "$NEW_HOME/.vscode"
    fi

    sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.config"
    
    # Ensure .gitconfig has correct ownership
    if [ -f "$NEW_HOME/.gitconfig" ]; then
        sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.gitconfig"
    fi
    
    # Replicate Git credentials
    if [ -f "$SOURCE_HOME/.git-credentials" ]; then
        sudo cp "$SOURCE_HOME/.git-credentials" "$NEW_HOME/.git-credentials"
        sudo chown "$NEW_USER:$NEW_USER" "$NEW_HOME/.git-credentials"
    fi
    
    # Ensure ACLs are correct on source for sharing
    _tus
}

_tuChromeAsYaniv() {
    _tuCleanLocks
    echo "Launching Chrome as 'yaniv' identity with VT-stability flags..."
    # Spoof identity while keeping current HOME to use the symlinked setup
    # Flags added to prevent tab discarding and GPU context loss reloads on VT switch
    USER=yaniv LOGNAME=yaniv google-chrome \
        --disable-backgrounding-occluded-windows \
        --disable-background-timer-throttling \
        --disable-renderer-backgrounding \
        --disable-gpu-process-crash-limit \
        "$@" &>/dev/null &
}

_tuCleanLocks() {
    local TARGET_DIR="/home/yaniv/.config/google-chrome"
    echo "Cleaning Chrome locks in $TARGET_DIR..."
    sudo find "$TARGET_DIR" -name "SingletonLock" -delete 2>/dev/null || true
    sudo find "$TARGET_DIR" -name "LOCK" -delete 2>/dev/null || true
    sudo find "$TARGET_DIR" -name "SingletonSocket" -delete 2>/dev/null || true
    sudo find "$TARGET_DIR" -name "SingletonCookie" -delete 2>/dev/null || true
}

_tur(){
    local TARGET_USER="$1"
    local TARGET_HOME="/home/$TARGET_USER"
    echo "Cleaning up $TARGET_USER and unmounting overlays..."
    
    # Unmount any active overlays (Lazy unmount to handle busy files)
    sudo umount -l "$TARGET_HOME/.config/google-chrome" 2>/dev/null || true
    sudo umount -l "$TARGET_HOME/.config/Code" 2>/dev/null || true
    sudo umount -l "$TARGET_HOME/.vscode" 2>/dev/null || true
    
    sudo pkill -9 -u "$TARGET_USER" || true
    sudo deluser --remove-home "$TARGET_USER"
    
    # Explicitly force remove the home directory if deluser left it behind
    # (This handles leftover overlay mount points or symlinks)
    if [ -d "$TARGET_HOME" ]; then
        echo "Force removing resident home directory $TARGET_HOME..."
        sudo rm -rf "$TARGET_HOME"
    fi
}

_theUserGroups(){
    groups $1
}

_tuMountOverlays() {
    local TARGET_USER="$1"
    local TARGET_HOME="/home/$TARGET_USER"
    local OVERLAY_BASE="$TARGET_HOME/.overlay"
    local SOURCE_HOME="/home/yaniv"
    local APPS=("google-chrome" "Code" ".vscode")

    if [ -d "$OVERLAY_BASE" ]; then
        for app in "${APPS[@]}"; do
            local LOWER_DIR="$SOURCE_HOME/.config/$app"
            [ "$app" == ".vscode" ] && LOWER_DIR="$SOURCE_HOME/$app"
            
            local TARGET_DIR="$TARGET_HOME/.config/$app"
            [ "$app" == ".vscode" ] && TARGET_DIR="$TARGET_HOME/$app"
            
            if [ -d "$LOWER_DIR" ] && [ -d "$TARGET_DIR" ]; then
                if ! mountpoint -q "$TARGET_DIR"; then
                    echo "Auto-mounting overlay for $app..."
                    local UPPER="$OVERLAY_BASE/$app/upper"
                    local WORK="$OVERLAY_BASE/$app/work"
                    sudo mount -t overlay overlay -o lowerdir="$LOWER_DIR",upperdir="$UPPER",workdir="$WORK" "$TARGET_DIR"
                fi
            fi
        done
    fi
}

_theUserSwitch(){
    _tuMountOverlays "$1"
    su - "$1"
}

_tuk(){
    # Hardcode users you never want to delete here:
    local HARDCODED_USERS=(
        "yaniv"
        # "userB"
    )
    
    local KEEP_USERS=("${HARDCODED_USERS[@]}" "$@")
    local PATTERN
    PATTERN="^($(IFS='|'; echo "${KEEP_USERS[*]}"))$"

    echo "Keeping protected users: ${KEEP_USERS[*]}"
    
    # 1. Remove active users from the system (using _tur)
    _tul | grep -vE "$PATTERN" | while read -r u; do
        echo "Removing active user $u..."
        _tur "$u"
    done

    # 2. Remove orphaned home directories (Ghost cleanup)
    # Scans /home for any directory that isn't in the KEEP_USERS list
    echo "Scanning for orphaned home directories..."
    sudo find /home -maxdepth 1 -mindepth 1 -type d | while read -r home_dir; do
        local user_name=$(basename "$home_dir")
        # Check if this directory name matches any preserved user
        if [[ ! " ${KEEP_USERS[*]} " =~ " ${user_name} " ]] && [ "$user_name" != "lost+found" ]; then
             echo "Found ghost directory: $home_dir (User: $user_name)"
             
             # Attempt lazy unmount just in case
             sudo umount -l "$home_dir/.config/google-chrome" 2>/dev/null || true
             sudo umount -l "$home_dir/.config/Code" 2>/dev/null || true
             sudo umount -l "$home_dir/.vscode" 2>/dev/null || true
             
             echo "Force removing orphaned directory $home_dir..."
             sudo rm -rf "$home_dir"
        fi
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
        "/home/$SOURCE_USER/.cache/google-chrome"
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
                echo "Recursively setting up ACLs for $dir (this may take a while)..."
                # sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$dir"
                sudo chmod -R g+rwx,g+s "$dir"
                sudo setfacl -R -b "$dir"
                sudo setfacl -R -m "u:$SOURCE_USER:rwx,g:$SHARED_GROUP:rwx,m:rwx" "$dir"
                sudo setfacl -R -d -m "u:$SOURCE_USER:rwx,g:$SHARED_GROUP:rwx,m:rwx" "$dir"
            else
                echo "Fast setup for $dir (parent directory and non-recursive ACLs)..."
                # sudo chown "$SOURCE_USER:$SHARED_GROUP" "$dir"
                sudo chmod g+rwx,g+s "$dir"
                # Set mask and group permission on the directory itself (fast)
                sudo setfacl -m "g:$SHARED_GROUP:rwx,m:rwx" "$dir"
                # Set DEFAULT ACL so NEW files in this folder are group-writable immediately
                sudo setfacl -d -m "u:$SOURCE_USER:rwx,g:$SHARED_GROUP:rwx,m:rwx" "$dir"
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
        # sudo chown "$SOURCE_USER:$SHARED_GROUP" "$SOCKET_DIR"
        sudo chmod g+rws "$SOCKET_DIR"
        sudo setfacl -m "g:$SHARED_GROUP:rx,m:rwx" "$SOCKET_DIR"
    fi
    if [ -S "$SOCKET" ]; then
        echo "Setting up permissions for daemon socket (making it globally accessible)..."
        # sudo chown "$SOURCE_USER:$SHARED_GROUP" "$SOCKET"
        sudo chmod 0666 "$SOCKET"
        sudo setfacl -m "g:$SHARED_GROUP:rw,m:rw,o:rw" "$SOCKET"
    fi

    if [ "$RECURSIVE" = false ]; then
        echo "💡 Tip: Run '_tus -a' if you need to fix permissions recursively (lengthy)."
    fi
}

_tuTakeConfig() {
    local TARGET_USER=$(whoami)
    local SHARED_GROUP="coding"
    local DIR="/home/yaniv/.config"
    if [ -d "$DIR" ]; then
        echo "Taking ownership of $DIR for $TARGET_USER..."
        # 1. Take ownership (Enabled by our new sudoers rule)
        sudo chown -R "$TARGET_USER:$SHARED_GROUP" "$DIR"
        # 2. Clear stale singleton locks
        sudo find "$DIR" -name "SingletonLock" -delete 2>/dev/null || true
        sudo find "$DIR" -name "LOCK" -delete 2>/dev/null || true
        # 3. Force permissions and mask reclamation
        sudo chmod -R g+rw "$DIR"
        sudo setfacl -R -m "m:rwx" "$DIR"
        echo "Done. You now have full parity over .config"
    fi
}

_tuFixConfig() {
    local SOURCE_USER="yaniv"
    local SHARED_GROUP="coding"
    local DIR="/home/$SOURCE_USER/.config"
    if [ -d "$DIR" ]; then
        echo "Granting 'Parity' to $SHARED_GROUP for all of $DIR..."
        # 1. Reclaim ownership (DISABLED as per user request)
        # sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$DIR"
        # 2. Force group permissions (restores masks)
        sudo chmod -R g+rw "$DIR"
        # 3. Force directories to be traversable and have setgid
        sudo find "$DIR" -type d -exec chmod 2770 {} +
        # 4. Force global ACL mask to rwx
        sudo setfacl -R -m "m:rwx" "$DIR"
        echo "Done. Full parity established for .config"
        
        # 5. EXPLICITLY fix the Preferences file which is stubborn
        local PREFS="$DIR/google-chrome/Default/Preferences"
        if [ -f "$PREFS" ]; then
             echo "Explicitly unmasking Preferences..."
             sudo chmod g+rw "$PREFS"
             sudo setfacl -m "m:rwx" "$PREFS"
             echo "Preferences status:"
             getfacl "$PREFS" | grep -E "user:|group:|mask:"
        fi
    fi
}

_tuFixChrome() {
    local SOURCE_USER="yaniv"
    local SHARED_GROUP="coding"
    local DIR="/home/$SOURCE_USER/.config/google-chrome"
    if [ -d "$DIR" ]; then
        echo "Breaking the circle: Fixing Chrome profile in $DIR..."
        # 1. Delete all lock files (stale or from other users)
        sudo find "$DIR" -name "SingletonLock" -delete 2>/dev/null || true
        sudo find "$DIR" -name "LOCK" -delete 2>/dev/null || true
        # 2. Reclaim ownership (DISABLED as per user request)
        # sudo chown -R "$SOURCE_USER:$SHARED_GROUP" "$DIR"
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

_tuMakeSuper(){
    echo "$(whoami) ALL=(ALL:ALL) ALL" | sudo tee /etc/sudoers.d/codingUsersSuper > /dev/null
}