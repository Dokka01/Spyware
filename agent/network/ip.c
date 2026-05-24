#include "ip.h"
#include "../config.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

void get_private_ip(char *ip, int size) {
    char hostname[BUFFER_SIZE];

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(ip, "Unknown", size - 1);
        return;
    }

    struct addrinfo hints = { 0 };
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *info;
    if (getaddrinfo(hostname, NULL, &hints, &info) == 0) {
        struct sockaddr_in *addr = (struct sockaddr_in *)info->ai_addr;
        strncpy(ip, inet_ntoa(addr->sin_addr), size - 1);
        freeaddrinfo(info);
    } else {
        strncpy(ip, "Unknown", size - 1);
    }
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total    = size * nmemb;
    char  *buf      = (char *)userdata;
    size_t cur_len  = strlen(buf);
    size_t capacity = BUFFER_SIZE - cur_len - 1;

    if (total > capacity)
        total = capacity;

    strncat(buf, (char *)ptr, total);
    return size * nmemb;
}

void get_public_ip(char *ip, int size) {
    ip[0] = '\0';

    CURL *curl = curl_easy_init();
    if (!curl) {
        strncpy(ip, "Unknown", size - 1);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ip);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        strncpy(ip, "Unknown", size - 1);

    curl_easy_cleanup(curl);
}
