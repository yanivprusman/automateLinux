defaultGnomeTerminalProfile=$( gsettings get org.gnome.Terminal.ProfilesList default | tr -d "'" )
gsettings set org.gnome.Terminal.Legacy.Profile:/org/gnome/terminal/legacy/profiles:/:"$defaultGnomeTerminalProfile"/ audible-bell false
unset defaultGnomeTerminalProfile