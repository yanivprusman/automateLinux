# Only run gsettings if DBUS is available (prevents hang when running as root or without desktop session)
if command -v gsettings >/dev/null && [ -n "$DBUS_SESSION_BUS_ADDRESS" ]; then
    if timeout 2 gsettings list-schemas 2>/dev/null | grep -q "org.gnome.Terminal.ProfilesList"; then
        defaultGnomeTerminalProfile=$( gsettings get org.gnome.Terminal.ProfilesList default 2>/dev/null | tr -d "'" )
        if [ -n "$defaultGnomeTerminalProfile" ]; then
             gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:"$defaultGnomeTerminalProfile"/ audible-bell false 2>/dev/null
        fi
        unset defaultGnomeTerminalProfile
    fi
fi