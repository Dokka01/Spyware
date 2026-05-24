#include "logs.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

void list_logs(const char *directory, char *output, int output_size) {
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", directory);

    WIN32_FIND_DATAA find_data;
    HANDLE hfind = FindFirstFileA(search_path, &find_data);

    if (hfind == INVALID_HANDLE_VALUE) {
        strncpy(output, "No logs found", output_size - 1);
        return;
    }

    output[0] = '\0';
    do {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        size_t remaining = output_size - strlen(output) - 2;
        if (remaining <= 0)
            break;

        strncat(output, find_data.cFileName, remaining);
        strncat(output, "\n", 1);
    } while (FindNextFileA(hfind, &find_data) != 0);

    FindClose(hfind);
}
