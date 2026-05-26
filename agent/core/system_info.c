#include "system_info.h"
#include <string.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

void get_hostname(char *hostname, int size) {
    if (gethostname(hostname, size) != 0)
        strncpy(hostname, "Unknown", size - 1);
}

void get_os_version(char *buf, int size) {
    RTL_OSVERSIONINFOW v = { sizeof(v) };

    /* RtlGetVersion (ntdll) retourne la vraie version Windows, contrairement à
       GetVersionEx qui est dépréciée et renvoie une valeur tronquée sur Win 8.1+ */
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    LONG (WINAPI *RtlGetVersion)(PRTL_OSVERSIONINFOW) =
        (void *)GetProcAddress(ntdll, "RtlGetVersion");

    if (RtlGetVersion && RtlGetVersion(&v) == 0) {
        snprintf(buf, size, "Windows %lu.%lu (Build %lu)",
                 v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
    } else {
        strncpy(buf, "Windows (inconnu)", size - 1);
    }
}
