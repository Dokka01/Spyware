from models.machine import Machine


def register_machine(data):
    hostname = data.get('hostname')
    os_info = data.get('os_info')

    if not hostname or not os_info:
        raise ValueError('hostname and os_info are required')

    Machine.create(
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
        raise ValueError(f'Machine {machine_id} not found')
    return machine
