#include "communication.h"
#include "../config.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

void send_report(const char *url, const MachineReport *report) {
    CURL *curl = curl_easy_init();
    if (!curl)
        return;

    char json[DATA_BUFFER];
    snprintf(json, sizeof(json),
             "{\"hostname\":\"%s\",\"os_info\":\"%s\","
             "\"private_ip\":\"%s\",\"public_ip\":\"%s\"}",
             report->hostname,
             report->os_info,
             report->private_ip,
             report->public_ip);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "[!] curl error: %s\n", curl_easy_strerror(res));

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
