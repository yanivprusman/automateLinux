_theUserList(){
    awk -F: '$3 >= 1000 && $3 < 60000 {print $1}' /etc/passwd
}

_theUserSetToBash(){
    sudo usermod -s /bin/bash $1
}

_theUserAddToCoding(){
    sudo usermod -aG coding $1
}

_theUserReplicateGnome(){
    local NEW_USER="$1"
    local SOURCE_USER="yaniv"
    local SOURCE_HOME="/home/$SOURCE_USER"
    local TARGET_HOME="/home/$NEW_USER"

    echo "Replicating GNOME settings from $SOURCE_USER to $NEW_USER..."

    # Replicate dconf settings
    sudo -u "$SOURCE_USER" dconf dump / > /tmp/yaniv-settings.conf
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf load / < /tmp/yaniv-settings.conf
    rm /tmp/yaniv-settings.conf

    # Disable welcome screen and set favorites
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf write /org/gnome/shell/welcome-dialog-last-shown-version "'99.9'"
    local FAVORITE_APPS="['code.desktop', 'google-chrome.desktop', 'libreoffice-writer.desktop', 'org.gnome.Terminal.desktop', 'com.mattjakeman.ExtensionManager.desktop', 'freecad.desktop', 'org.gnome.GPaste.Ui.desktop', 'barrier.desktop', 'discord_discord.desktop', 'thunderbird_thunderbird.desktop', 'org.gnome.Nautilus.desktop', 'snap-store_snap-store.desktop', 'yelp.desktop', 'org.gnome.Evince.desktop', 'org.gnome.Settings.desktop', 'org.gnome.Calculator.desktop', 'postman_postman.desktop', 'tableplus.desktop', 'org.gnome.Extensions.desktop', 'org.kde.krita.desktop', 'realvnc-vncviewer.desktop']"
    sudo -i -u "$NEW_USER" dbus-run-session -- dconf write /org/gnome/shell/favorite-apps "$FAVORITE_APPS"

    # Copy GNOME config files
    for ver in 3.0 4.0; do
        if [ -d "$SOURCE_HOME/.config/gtk-$ver" ]; then
            sudo mkdir -p "$TARGET_HOME/.config/gtk-$ver"
            sudo cp -r "$SOURCE_HOME/.config/gtk-$ver/"* "$TARGET_HOME/.config/gtk-$ver/" 2>/dev/null || true
        fi
    done
    
    # Shell Extensions
    if [ -d "$SOURCE_HOME/.local/share/gnome-shell/extensions" ]; then
        sudo mkdir -p "$TARGET_HOME/.local/share/gnome-shell/extensions"
        sudo cp -r "$SOURCE_HOME/.local/share/gnome-shell/extensions/"* "$TARGET_HOME/.local/share/gnome-shell/extensions/" 2>/dev/null || true
    fi

    # Fix ownership
    sudo chown -R "$NEW_USER:$NEW_USER" "$TARGET_HOME"
}

_theUserCreate(){
    sudo adduser "$1"
    _theUserSetToBash "$1"
    _theUserAddToCoding "$1"
    _theUserReplicateGnome "$1"
}

_theUserRemove(){
    sudo pkill -9 -u $1 || true
    sudo deluser --remove-home $1
}

_theUserGroups(){
    groups $1
}

_theUserSwitch(){
    su - $1
}

