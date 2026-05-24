#ifndef COMMUNICATION_H
#define COMMUNICATION_H

typedef struct {
    const char *hostname;
    const char *os_info;
    const char *private_ip;
    const char *public_ip;
} MachineReport;

void send_report(const char *url, const MachineReport *report);

#endif /* COMMUNICATION_H */
