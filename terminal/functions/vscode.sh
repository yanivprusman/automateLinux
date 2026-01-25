commentVscodeRootCheck() {
    sudo sed -i '
/^# If root, ensure that --user-data-dir or --file-write is specified/!b
:loop
s/^\([^#]\)/#\1/
n
/^[^#].*fi/{
s/^\([^#]\)/#\1/
b
}
b loop
' /usr/bin/code
}

uncommentVscodeRootCheck() {
    sudo sed -i '
/^# If root, ensure that --user-data-dir or --file-write is specified/!b
:loop
s/^#//
n
/^#.*fi/{
s/^#//
b
}
b loop
' /usr/bin/code
}
