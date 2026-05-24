import sqlite3
from config import Config


def get_db():
    conn = sqlite3.connect(Config.DATABASE)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    conn = get_db()
    conn.execute('''
        CREATE TABLE IF NOT EXISTS machines (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            hostname   TEXT    NOT NULL,
            os_info    TEXT    NOT NULL,
            private_ip TEXT,
            public_ip  TEXT,
            last_seen  DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()
