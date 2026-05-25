import os
import sqlite3
from config import Config

# En Docker, le dossier contenant la DB peut ne pas exister encore → on le crée
os.makedirs(os.path.dirname(os.path.abspath(Config.DATABASE)), exist_ok=True)


def get_db():
    """Ouvre et retourne une connexion à la base de données SQLite."""
    conn = sqlite3.connect(Config.DATABASE)
    # Row factory : permet d'accéder aux colonnes par nom (row['hostname'] au lieu de row[1])
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    """
    Initialise la base de données au démarrage du serveur.
    - Crée la table 'machines' si elle n'existe pas.
    - Ajoute la colonne 'last_seen' si elle manque (migration pour les anciennes DBs).
    """
    conn = get_db()

    conn.execute('''
        CREATE TABLE IF NOT EXISTS machines (
            id         INTEGER  PRIMARY KEY AUTOINCREMENT,
            hostname   TEXT     NOT NULL,
            os_info    TEXT     NOT NULL,
            private_ip TEXT,
            public_ip  TEXT,
            last_seen  DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    existing_columns = {row[1] for row in conn.execute('PRAGMA table_info(machines)').fetchall()}
    for col, definition in (
        ('private_ip',      'TEXT'),
        ('public_ip',       'TEXT'),
        ('last_seen',       'DATETIME DEFAULT CURRENT_TIMESTAMP'),
        ('pending_command', 'TEXT'),
        ('command_result',  'TEXT'),
    ):
        if col not in existing_columns:
            conn.execute(f'ALTER TABLE machines ADD COLUMN {col} {definition}')

    conn.commit()
    conn.close()
