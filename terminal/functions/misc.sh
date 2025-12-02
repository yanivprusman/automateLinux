# Miscellaneous utilities

password() { python3 ~/generatePassword.py; }

h() {
    history | grep "$@"
}
export -f h

rmr() {
    rm -r *
}
export -f rmr

