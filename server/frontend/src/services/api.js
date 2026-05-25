// URL de base des routes machines du backend
const BASE_URL = '/api/machines'

// Récupère toutes les machines enregistrées
export async function fetchMachines() {
  const res = await fetch(BASE_URL)
  if (!res.ok) throw new Error(`Erreur ${res.status} : impossible de récupérer les machines`)
  return res.json()
}

// Récupère une machine spécifique par son ID
export async function fetchMachine(id) {
  const res = await fetch(`${BASE_URL}/${id}`)
  if (!res.ok) throw new Error(`Erreur ${res.status} : machine ${id} introuvable`)
  return res.json()
}

// Envoie une commande à une machine (déclenche une action sur l'agent)
export async function sendCommand(id, command) {
  const res = await fetch(`${BASE_URL}/${id}/command`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ command }),
  })
  if (!res.ok) throw new Error(`Erreur envoi commande : ${res.status}`)
  return res.json()
}

// Récupère le résultat de la dernière commande exécutée par l'agent
export async function fetchResult(id) {
  const res = await fetch(`${BASE_URL}/${id}/result`)
  if (!res.ok) throw new Error(`Erreur résultat : ${res.status}`)
  return res.json() // { result: "..." | null }
}

// Retourne l'URL de téléchargement d'un fichier uploadé par l'agent
export function getDownloadUrl(id, filename) {
  return `${BASE_URL}/${id}/download/${filename}`
}
