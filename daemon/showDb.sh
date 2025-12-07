#!/bin/bash
DB="${AUTOMATE_LINUX_DATA_DIR}daemon.db"
echo "Tables:"
sqlite3 "$DB" ".tables"
echoBlockSeparator
for t in $(sqlite3 "$DB" ".tables"); do
    echo "=== $t ==="
    echo "-- columns --"
    sqlite3 "$DB" "PRAGMA table_info($t);"
    echo "-- rows --"
    sqlite3 "$DB" "SELECT * FROM $t;"
    echoBlockSeparator
done
