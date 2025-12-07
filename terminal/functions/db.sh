
watchDb() {
    while inotifywait -e modify "${AUTOMATE_LINUX_DIR}data/daemon.db" > /dev/null 2>&1; do
        d printDirHistory > "${AUTOMATE_LINUX_DIR}data/watchDB.txt"
        # d printDirHistory
    done
}