_theUserList(){
    awk -F: '$3 >= 1000 && $3 < 60000 {print $1}' /etc/passwd
}

_theUserSetToBash(){
    sudo usermod -s /bin/bash $1
}

_theUserAddToCoding(){
    sudo usermod -aG coding $1
}

_theUserCreate(){
    sudo adduser $1
    _theUserSetToBash $1
    _theUserAddToCoding $1
}

_theUserRemove(){
    sudo deluser --remove-home $1
}

_theUserGroups(){
    groups $1
}

_theUserSwitch(){
    su - $1
}

