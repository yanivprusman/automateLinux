#!/bin/bash

# Configuration
BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
MYSQL_DIR="$BASE_DIR/data/mysql"
CONF_FILE="$MYSQL_DIR/conf/my.cnf"

if [ ! -f "$CONF_FILE" ]; then
    echo "MySQL configuration file not found at $CONF_FILE"
    echo "Is the daemon running?"
    exit 1
fi

PORT=$(grep "port=" "$CONF_FILE" | cut -d= -f2 | head -n 1)
USER="automate_user"
PASSWORD="automate_password"
DATABASE="automate_db"

# Check if mysql client is installed
if ! command -v mysql &> /dev/null; then
    echo "Error: 'mysql' command not found. Please install mysql-client."
    exit 1
fi

run_sql() {
    mysql -h 127.0.0.1 -P "$PORT" -u "$USER" --password="$PASSWORD" "$DATABASE" "$@" 2>/dev/null
}

if [ -z "$1" ]; then
    echo "--- Database Summary ---"
    run_sql -N -e "SHOW TABLES;" | while read table; do
        count=$(run_sql -N -e "SELECT COUNT(*) FROM $table;")
        printf "%-25s | %d rows\n" "$table" "$count"
    done
    echo "------------------------"
    echo "Usage: $0 [table_name]"
else
    TABLE_NAME="$1"
    echo "--- Table: $TABLE_NAME ---"
    
    # Check if table exists
    EXISTS=$(run_sql -N -e "SHOW TABLES LIKE '$TABLE_NAME';")
    if [ -z "$EXISTS" ]; then
        echo "Error: Table '$TABLE_NAME' not found."
        exit 1
    fi

    # Specialized viewing for terminal_history
    if [ "$TABLE_NAME" == "terminal_history" ]; then
        run_sql -e "SELECT tty, entry_index, path FROM terminal_history ORDER BY tty ASC, entry_index ASC;"
    elif [ "$TABLE_NAME" == "terminal_sessions" ]; then
         run_sql -e "SELECT tty, history_index as current_ptr FROM terminal_sessions ORDER BY tty ASC;"
    else
        run_sql -e "SELECT * FROM $TABLE_NAME;"
    fi
fi
