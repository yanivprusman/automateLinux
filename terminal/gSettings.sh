if command -v gsettings >/dev/null; then
    {
        if gsettings list-schemas | grep -q "org.gnome.Terminal.ProfilesList"; then
            defaultGnomeTerminalProfile=$( gsettings get org.gnome.Terminal.ProfilesList default | tr -d "'" )
            if [ -n "$defaultGnomeTerminalProfile" ]; then
                 gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:"$defaultGnomeTerminalProfile"/ audible-bell false
            fi
        fi
    } &
    disown
fi
