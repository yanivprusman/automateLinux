# Miscellaneous utilities

generatePassword() { python3 ~/generatePassword.py; }

h() {
    history | grep "$@"
}
export -f h
