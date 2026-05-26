# Spyware C2 — Contexte projet

## But du projet

Projet étudiant de cybersécurité (POC — Proof of Concept).
On simule une infrastructure C2 (Command & Control) pour comprendre comment fonctionnent les logiciels malveillants côté technique.

L'architecture est volontairement simple et pédagogique : pas d'authentification, pas de chiffrement avancé, juste le strict nécessaire pour que ça tourne.

## Les trois composants

### 1. Agent (`/agent/`)
- Écrit en C, compilé avec MinGW pour Windows
- Ce que fait l'agent au lancement :
  1. Récupère le hostname de la machine
  2. Récupère la version de l'OS
  3. Récupère l'IP privée (réseau local)
  4. Récupère l'IP publique via l'API `api.ipify.org`
  5. Envoie tout ça en JSON au backend via une requête HTTP POST
  6. Liste les logs Windows (`C:\Windows\Logs`)
  7. Prend un screenshot (sauvegardé en `.bmp`)
- Dépendances C : libcurl, Winsock2, GDI32
- Compile avec : `make all` dans `/agent/`

### 2. Backend (`/server/backend/`)
- API REST en Flask (Python)
- Base de données SQLite (`spyware.db`)
- Tourne sur `http://0.0.0.0:5000`
- Routes disponibles :
  - `POST /api/machines/report` — l'agent envoie ses données ici
  - `GET  /api/machines`        — le frontend récupère la liste des machines
  - `GET  /api/machines/<id>`   — détails d'une machine spécifique

### 3. Frontend (`/server/frontend/`)
- Dashboard React + Vite + Tailwind CSS v4 + shadcn/ui
- Tourne sur `http://localhost:5173` en dev
- Affiche en temps réel la liste des machines qui ont check-in
- Fonctionnalités : statut en direct, recherche, actions par machine (à implémenter)

## Structure des fichiers

```
Spyware/
├── agent/
│   ├── agent.c              # Point d'entrée de l'agent
│   ├── config.h             # URL du serveur C2, constantes
│   ├── Makefile
│   ├── core/
│   │   ├── system_info.h/c  # Hostname + version OS
│   │   ├── screenshot.h/c   # Capture écran
│   │   └── logs.h/c         # Enumération des logs Windows
│   └── network/
│       ├── communication.h/c # Envoi JSON via libcurl
│       └── ip.h/c            # Récupération IP privée/publique
└── server/
    ├── docker-compose.yml    # Lance backend + frontend en containers
    ├── backend/
    │   ├── run.py            # Point d'entrée Flask (python run.py)
    │   ├── app.py            # Factory Flask (create_app)
    │   ├── config.py         # Config (chemin DB, debug)
    │   ├── database.py       # Connexion SQLite + init table
    │   ├── models/
    │   │   └── machine.py    # Classe Machine (CRUD SQLite)
    │   ├── routes/
    │   │   └── machines.py   # Routes Flask /api/machines/*
    │   ├── services/
    │   │   └── machine_service.py # Logique métier (validation, etc.)
    │   ├── Dockerfile
    │   └── requirements.txt
    └── frontend/
        ├── src/
        │   ├── main.jsx               # Point d'entrée React
        │   ├── App.jsx                # Layout général + routing
        │   ├── index.css              # Thème dark + variables CSS
        │   ├── services/
        │   │   └── api.js             # Appels fetch vers le backend
        │   ├── lib/
        │   │   └── utils.js           # Fonction cn() pour Tailwind
        │   ├── components/
        │   │   ├── MachineTable.jsx   # Tableau des machines
        │   │   ├── layout/
        │   │   │   └── Sidebar.jsx    # Barre de navigation latérale
        │   │   └── ui/                # Composants shadcn/ui
        │   │       ├── badge.jsx
        │   │       ├── button.jsx
        │   │       ├── card.jsx
        │   │       ├── dropdown-menu.jsx
        │   │       ├── input.jsx
        │   │       ├── separator.jsx
        │   │       ├── skeleton.jsx
        │   │       └── table.jsx
        │   └── pages/
        │       ├── Dashboard.jsx      # Page principale (stats + tableau)
        │       └── Settings.jsx       # Référence config + stack
        ├── Dockerfile
        ├── nginx.conf                 # Config nginx pour la prod Docker
        └── package.json
```

## Comment lancer le projet en dev

### Backend
```bash
cd server/backend
python -m venv venv
venv\Scripts\activate       # Windows
pip install -r requirements.txt
python run.py
# → http://localhost:5000
```

### Frontend
```bash
cd server/frontend
npm install
npm run dev
# → http://localhost:5173
```

### Via Docker (preprod)
```bash
cd server
docker compose up --build
# Frontend → http://localhost:80
# Backend  → http://localhost:5000
```

## Conventions de code

- Code en **anglais** (noms de variables, fonctions, commentaires techniques)
- **Style étudiant** : clair, simple, commenté là où c'est pas évident
- Pas d'abstraction inutile — si une fonction n'est utilisée qu'une fois, pas besoin de la sortir
- Les composants React utilisent des **hooks simples** (useState, useEffect), pas de contexte ou store global
- Le backend est structuré en 3 couches : `routes` → `services` → `models`, mais c'est suffisant pour ce POC
- Pas d'authentification (c'est un POC, pas de la prod)

## Travail futur (idées)

- [ ] Ajouter une route pour récupérer le screenshot d'une machine
- [ ] Ajouter une route pour récupérer les logs d'une machine
- [ ] Implémenter les boutons "Screenshot" et "Fetch Logs" dans le frontend
- [ ] Ajouter un système de tags/groupes pour les machines
- [ ] Authentification basique sur le backend
