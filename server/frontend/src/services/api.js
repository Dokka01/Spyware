const BASE = '/api/machines'

export async function fetchMachines() {
  const res = await fetch(BASE)
  if (!res.ok) throw new Error(`Failed to fetch machines: ${res.status}`)
  return res.json()
}

export async function fetchMachine(id) {
  const res = await fetch(`${BASE}/${id}`)
  if (!res.ok) throw new Error(`Failed to fetch machine ${id}: ${res.status}`)
  return res.json()
}
