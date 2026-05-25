#include "installed_apps.h"
#include "../config.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void sanitize(char *s) {
    for (; *s; s++)
        if (*s == '"' || *s == '\\') *s = '\'';
}

void get_installed_apps(char *out, size_t out_size) {
    HKEY hKey;
    const char *reg_path =
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_path, 0,
                     KEY_READ | KEY_WOW64_32KEY, &hKey) != ERROR_SUCCESS) {
        snprintf(out, out_size,
                 "[{\"name\":\"Registre inaccessible\",\"version\":\"\"}]");
        return;
    }

    size_t pos = 0;
    pos += snprintf(out + pos, out_size - pos, "[");
    int first = 1;
    int index = 0;

    char subkey_name[256];
    DWORD subkey_len = sizeof(subkey_name);

    while (RegEnumKeyEx(hKey, index++, subkey_name, &subkey_len,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        subkey_len = sizeof(subkey_name);

        HKEY hSub;
        if (RegOpenKeyEx(hKey, subkey_name, 0, KEY_READ, &hSub) != ERROR_SUCCESS)
            continue;

        char name[256]    = {0};
        char version[64]  = {0};
        DWORD name_sz     = sizeof(name);
        DWORD version_sz  = sizeof(version);
        DWORD type;

        RegQueryValueEx(hSub, "DisplayName",    NULL, &type, (LPBYTE)name,    &name_sz);
        RegQueryValueEx(hSub, "DisplayVersion", NULL, &type, (LPBYTE)version, &version_sz);
        RegCloseKey(hSub);

        if (name[0] == '\0') continue;

        sanitize(name);
        sanitize(version);

        if (!first) pos += snprintf(out + pos, out_size - pos, ",");
        first = 0;
        pos += snprintf(out + pos, out_size - pos,
                        "{\"name\":\"%s\",\"version\":\"%s\"}", name, version);

        if (pos >= out_size - 512) break;
    }

    snprintf(out + pos, out_size - pos, "]");
    RegCloseKey(hKey);
}
