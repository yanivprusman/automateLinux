# watchDb() {
#     watch -d -n 1 "sqlite3 \"$AUTOMATE_LINUX_DIR/data/daemon.db\" 'SELECT * FROM kv;'"
# }

# watchDb() {
#     inotifywait -m -e modify "$AUTOMATE_LINUX_DIR/data/daemon.db" |
#     while read -r _; do
#         . "${AUTOMATE_LINUX_DAEMON_DIR}dbChanged.sh"
#     done
#     #     d printDirHistory > "${AUTOMATE_LINUX_DIR}/data/watchDB.txt"
#     #     # sqlite3 "$AUTOMATE_LINUX_DIR/data/daemon.db" "SELECT * FROM kv;" > "${AUTOMATE_LINUX_DIR}/data/watchDB.txt"

# }
# export -f watchDb

watchDb() {
    while inotifywait -e modify "${AUTOMATE_LINUX_DIR}data/daemon.db"; do
        d printDirHistory > "${AUTOMATE_LINUX_DIR}data/watchDB.txt"
    done
}