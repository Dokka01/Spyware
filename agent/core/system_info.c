#include "system_info.h"
#include <string.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

void get_hostname(char *hostname, int size) {
    if (gethostname(hostname, size) != 0)
        strncpy(hostname, "Unknown", size - 1);
}

/*
 * Uses RtlGetVersion via a runtime import to bypass the deprecated
 * GetVersionEx manifest requirement on Windows 8.1+.
 */
void get_os_version(char *os_info, int size) {
    typedef LONG (WINAPI *RtlGetVersionFunc)(PRTL_OSVERSIONINFOW);

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    RtlGetVersionFunc RtlGetVersion =
        (RtlGetVersionFunc)GetProcAddress(ntdll, "RtlGetVersion");

    if (RtlGetVersion) {
        RTL_OSVERSIONINFOW osvi = { sizeof(RTL_OSVERSIONINFOW) };
        if (RtlGetVersion(&osvi) == 0) {
            snprintf(os_info, size, "Windows %lu.%lu (Build %lu)",
                     osvi.dwMajorVersion,
                     osvi.dwMinorVersion,
                     osvi.dwBuildNumber);
            return;
        }
    }

    strncpy(os_info, "Windows (Unknown)", size - 1);
}
