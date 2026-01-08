_theUserList(){
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

_theUserCreate(){
    local NEW_USER="$1"
    local SOURCE_USER="yaniv"
    local NEW_HOME="/home/$NEW_USER"
    # sudo adduser "$NEW_USER"
    sudo useradd -m "$NEW_USER"
    echo "$NEW_USER:\\" | sudo chpasswd
    _theUserSetToBash "$NEW_USER"
    _theUserAddToCoding "$NEW_USER"
    if [ ! -d "$NEW_HOME" ]; then
        sudo mkdir -p "$NEW_HOME"
    fi
    sudo chown -R "$NEW_USER:$NEW_USER" "$NEW_HOME"
    sudo chmod 755 "$NEW_HOME"
    sudo -u "$NEW_USER" mkdir -p "$NEW_HOME/.config"
    sudo -u "$NEW_USER" touch "$NEW_HOME/.config/gnome-initial-setup-done"
    __theUserReplicateGnome "$NEW_USER"
    sudo chown -R $NEW_USER:yaniv /home/yaniv/.config/Code
    sudo chown -R $NEW_USER:yaniv /home/yaniv/.config/google-chrome
    sudo rm -rf /home/$NEW_USER/.config/Code
    sudo ln -s /home/yaniv/.config/Code /home/$NEW_USER/.config/Code
    sudo rm -rf /home/$NEW_USER/.config/google-chrome
    sudo ln -s /home/yaniv/.config/google-chrome /home/$NEW_USER/.config/google-chrome
}

_theUserRemove(){
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

