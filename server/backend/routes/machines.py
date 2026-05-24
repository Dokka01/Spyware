from flask import Blueprint, request, jsonify
from services.machine_service import register_machine, list_machines, get_machine

machines_bp = Blueprint('machines', __name__)


@machines_bp.route('/report', methods=['POST'])
def report():
    data = request.get_json()
    if not data:
        return jsonify({'error': 'Invalid JSON body'}), 400

    try:
        register_machine(data)
        return jsonify({'message': 'Report received'}), 201
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
