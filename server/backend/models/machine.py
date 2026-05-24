from database import get_db


class Machine:
    @staticmethod
    def create(hostname, os_info, private_ip=None, public_ip=None):
        db = get_db()
        db.execute(
            'INSERT INTO machines (hostname, os_info, private_ip, public_ip) VALUES (?, ?, ?, ?)',
            (hostname, os_info, private_ip, public_ip)
        )
        db.commit()
        db.close()

    @staticmethod
    def get_all():
        db = get_db()
        rows = db.execute('SELECT * FROM machines ORDER BY last_seen DESC').fetchall()
        db.close()
        return [dict(row) for row in rows]

    @staticmethod
    def get_by_id(machine_id):
        db = get_db()
        row = db.execute('SELECT * FROM machines WHERE id = ?', (machine_id,)).fetchone()
        db.close()
        return dict(row) if row else None
