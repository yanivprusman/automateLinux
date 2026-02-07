if command -v gsettings >/dev/null; then
    {
        if gsettings list-schemas | grep -q "org.gnome.Terminal.ProfilesList"; then
            defaultGnomeTerminalProfile=$( gsettings get org.gnome.Terminal.ProfilesList default | tr -d "'" )
            if [ -n "$defaultGnomeTerminalProfile" ]; then
                 gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:"$defaultGnomeTerminalProfile"/ audible-bell false
            fi
        fi
        if gsettings list-schemas | grep -q "org.gnome.Ptyxis"; then
            gsettings set org.gnome.Ptyxis use-system-font false
            gsettings set org.gnome.Ptyxis font-name 'Monospace 16'
            gsettings set org.gnome.Ptyxis audible-bell false
            gsettings set org.gnome.Ptyxis interface-style 'dark'
        fi
    } &
    disown
fi
