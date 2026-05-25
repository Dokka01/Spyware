# Design — C2 Features & Machine Detail UI

Date: 2026-05-25

## Contexte

Le projet est un POC étudiant de C2 (Command & Control). L'agent C tourne sur la machine cible, le backend est Flask/SQLite, le frontend est React + Vite + Tailwind + shadcn/ui.

Actuellement l'agent s'exécute une fois (envoie hostname/OS/IPs) puis quitte. On veut ajouter des features déclenchables à la demande depuis le WebUI.

---

## Nouvelles fonctionnalités

### Côté agent (C)
- **Keylogger** : start/stop à distance, envoie le buffer toutes les 15 min
- **Screenshot** : capture à la demande, envoi au serveur en PNG (ou BMP)
- **Browser history** : lit le SQLite de Chrome dans AppData, renvoie JSON
- **Installed apps** : lit le registre Windows (HKLM\...\Uninstall), renvoie JSON
- **File enum + exfil** : liste ou ZIP un dossier cible (Documents, Images, Videos, Bureau), upload au serveur

### Côté frontend
- Clic sur une ligne du tableau → page `MachineDetail` (`/machine/:id`)
- Boutons d'action par feature, résultats affichés en dessous
- Keylogger : bouton Start / Stop avec état visuel

---

## Architecture — Système de commandes

### Modèle retenu : polling loop + colonne pending_command

Simple, lisible, sans WebSocket. L'agent poll toutes les 30s.

**2 colonnes ajoutées dans la table `machines` :**
- `pending_command TEXT` — commande en attente (ex: `"screenshot"`, `"start_keylog"`, `"browser_history"`, `"apps"`, `"files:Documents"`)
- `command_result TEXT` — résultat renvoyé par l'agent (JSON ou texte)

Pour les fichiers binaires (ZIP, BMP), un dossier `server/backend/uploads/<machine_id>/` stocke les fichiers. Le `command_result` contient le chemin relatif.

**Nouvelles routes backend :**
```
GET  /api/machines/{id}/command        → l'agent poll (renvoie pending_command ou null)
POST /api/machines/{id}/command        → le frontend envoie une commande
POST /api/machines/{id}/result         → l'agent envoie le résultat texte
POST /api/machines/{id}/upload         → l'agent upload un fichier (multipart)
GET  /api/machines/{id}/download/<f>   → le frontend télécharge un fichier
```

### Flow d'une commande

```
1. Opérateur clique "Screenshot" dans l'UI
2. Frontend : POST /api/machines/{id}/command  { "command": "screenshot" }
3. Backend : écrit "screenshot" dans pending_command
4. Agent (30s max) : GET /api/machines/{id}/command → reçoit "screenshot"
5. Agent : exécute la capture, POST /api/machines/{id}/upload (multipart)
6. Backend : sauvegarde le fichier, écrit le chemin dans command_result, vide pending_command
7. Frontend : poll le résultat, affiche l'image
```

---

## Agent C — Structure des modules

### Refactoring de agent.c
- Boucle infinie : envoie rapport initial, puis `while(1) { poll_command(); Sleep(30000); }`
- Dispatch par string de commande : `strcmp(cmd, "screenshot")`, etc.

### Modules existants conservés
- `network/communication.c` — on y ajoute `get_command()`, `post_result()`, `upload_file()`
- `core/screenshot.c` — existant, on ajoute juste l'appel depuis le dispatch
- `core/system_info.c` — inchangé
- `network/ip.c` — inchangé

### Modules supprimés
- `core/logs.c` / `core/logs.h` — inutilisé côté serveur, supprimé

### Nouveaux modules
- `core/keylogger.c/.h` — SetWindowsHookEx WH_KEYBOARD_LL dans un thread, buffer de frappes, envoi toutes les 15 min via upload_file
- `core/browser_history.c/.h` — copie le fichier SQLite Chrome (`History`), le lit avec sqlite3.h, renvoie les 50 dernières URLs en JSON
- `core/installed_apps.c/.h` — RegOpenKeyEx sur `HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall`, itère les sous-clés, renvoie JSON
- `core/file_enum.c/.h` — liste les fichiers d'un dossier (Documents, Images, Videos, Bureau) et ZIP via minizip, upload le zip

---

## Backend Flask — Ce qui change

### database.py
Migration : ajoute `pending_command TEXT` et `command_result TEXT` à la table `machines`.

### models/machine.py
- `set_command(machine_id, command)` — écrit pending_command
- `get_command(machine_id)` — lit pending_command
- `set_result(machine_id, result)` — écrit command_result, vide pending_command
- `get_result(machine_id)` — lit command_result

### services/machine_service.py
- `send_command(machine_id, command)` — valide la commande et délègue au model
- `fetch_command(machine_id)` — pour l'agent
- `store_result(machine_id, result)` — pour l'agent
- `get_result(machine_id)` — pour le frontend

### routes/machines.py
Nouvelles routes ajoutées au blueprint existant.

---

## Frontend React — Ce qui change

### App.jsx
Nouvelle route : `<Route path="/machine/:id" element={<MachineDetail />} />`

### MachineTable.jsx
- Chaque ligne devient cliquable → `navigate('/machine/${machine.id}')`
- Suppression du DropdownMenu (remplacé par la page détail)

### pages/MachineDetail.jsx (nouveau)
Sections :
1. **Header** — hostname, OS badge, statut, IPs, last seen
2. **Screenshot** — bouton "Capture", affiche l'image si résultat disponible
3. **Keylogger** — bouton Start (vert) / Stop (rouge), affiche le log reçu
4. **Browser History** — bouton "Fetch", affiche tableau d'URLs
5. **Installed Apps** — bouton "Fetch", affiche liste d'apps
6. **File Exfiltration** — 4 boutons (Documents, Images, Videos, Bureau), lien de téléchargement ZIP

### services/api.js
Nouvelles fonctions : `sendCommand(id, command)`, `fetchResult(id)`, `getDownloadUrl(id, filename)`

---

## Nettoyage prévu

- Supprimer `core/logs.c`, `core/logs.h`, retirer l'include et l'appel dans `agent.c`
- Supprimer le DropdownMenu de `MachineTable.jsx` (remplacé par navigation)
- Supprimer les imports inutilisés (`FileText`, `MoreHorizontal` dans MachineTable)

---

## Contraintes

- Code simple, style étudiant : pas d'abstraction inutile
- Pas d'authentification (POC)
- Agent compilé MinGW pour Windows
- Dépendances C : libcurl (déjà là), minizip (pour les ZIPs), sqlite3.h (pour browser history)
- Frontend : réutilise les composants shadcn/ui existants, hooks simples (useState, useEffect)
