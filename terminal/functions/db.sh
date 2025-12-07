# watchDb() {
#     watch -d -n 1 "sqlite3 \"$AUTOMATE_LINUX_DIR/data/daemon.db\" 'SELECT * FROM kv;'"
# }

watchDb() {
    inotifywait -m -e modify "$AUTOMATE_LINUX_DIR/data/daemon.db" |
    while read -r _; do
        sqlite3 "$AUTOMATE_LINUX_DIR/data/daemon.db" "SELECT * FROM kv;" > "${AUTOMATE_LINUX_DIR}/data/watchDB.txt"
    done
}
export -f watchDb
