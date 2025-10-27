if ! pgrep "gnome-terminal" > /dev/null; then
    # No terminal found, launch a new one
    gtk-launch org.gnome.Terminal.desktop
    echo "Launching new terminal"
else
    echo "Terminal is already running"
fi
