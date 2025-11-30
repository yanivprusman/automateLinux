#!/bin/bash
DB="../data/daemon.db"

sqlite3 "$DB" <<EOF
DELETE FROM history;
DELETE FROM terminals;
VACUUM;

INSERT INTO history (tty, idx, directory)
VALUES
 ('tty1', 1, '/home/yaniv'),
 ('tty2', 2, '/var/log');

INSERT INTO terminals (tty, last_changed, is_active)
VALUES
 ('tty1', strftime('%s','now'), 1),
 ('tty2', strftime('%s','now'), 0);
EOF

echo "done"
