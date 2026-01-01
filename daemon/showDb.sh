#!/bin/bash

# Configuration
BASE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
MYSQL_DIR="$BASE_DIR/data/mysql"
CONF_FILE="$MYSQL_DIR/conf/my.cnf"
MYSQLD_CUSTOM="$MYSQL_DIR/bin/mysqld"

if [ ! -f "$CONF_FILE" ]; then
    echo "MySQL configuration file not found at $CONF_FILE"
    echo "Is the daemon running?"
    exit 1
fi

# Extract port/socket from config if needed, or just let mysql client use the socket defined in my.cnf if explicit
# But mysql client might verify defaults-file differently.
# Easiest way is to parse port from my.cnf
PORT=$(grep "port=" "$CONF_FILE" | cut -d= -f2)
USER="automate_user"
PASSWORD="automate_password"
DATABASE="automate_db"

echo "Connecting to MySQL on port $PORT..."

# Check if mysql client is installed
if ! command -v mysql &> /dev/null; then
    echo "Error: 'mysql' command not found. Please install mysql-client."
    exit 1
fi

mysql -h 127.0.0.1 -P "$PORT" -u "$USER" -p"$PASSWORD" "$DATABASE" -e "SELECT * FROM kv;"
